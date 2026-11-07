# 15 · Reflection and Serialization

## What it is

**Reflection** is knowing, at runtime, the fields of a component: their names, types and positions in
memory. **Serialization** uses that knowledge to turn component bytes into a portable form (a network
packet, a save file, a Luau table) and back.

## Why we need it

One description of the fields, many users:

| User | Needs |
|---|---|
| [Replication](20-replication.md) (R3) | Write any replicated component into a packet, compactly |
| [Luau bridge](19-luau-bridge.md) (R2) | Convert between component bytes and Luau tables; read and write single fields |
| [Actors and scenes](18-actors-and-scenes.md) | Store component values by field name |
| Debug tools (R4) | Show and edit any component in an inspector |

Without reflection, each of these would need hand-written code per component, and Luau components would
not work at all.

## How others do it

| ECS / engine | Reflection |
|---|---|
| flecs | `meta` addon: field descriptions registered at runtime; JSON serialization built in |
| Bevy | `#[derive(Reflect)]` derive macro; `bevy_reflect` serializers |
| Unreal | `UPROPERTY` macros parsed by the Unreal Header Tool; drives editor, saves, replication, Blueprint |
| EnTT | `entt::meta`, registered by hand |
| C++26 | Static reflection (P2996), not yet available on all our compilers |

## Our choice

**One `FieldInfo` list per component**, stored in its [`ComponentInfo`](02-component-registry.md):

- from C++, written once by `RTYPE_ECS_COMPONENT(Type, "name", field1, field2, ...)`;
- from Luau, read from the component's `export type` (see [Luau bridge](19-luau-bridge.md)).

**One generic serializer** walks the field list. Nothing is serialized per type by hand.

## How it works

### Field descriptions

```c++
struct Transform {
    glm::vec2 position;
    float rotation;
    glm::vec2 scale;
};

RTYPE_ECS_COMPONENT(Transform, "rtype.Transform", position, rotation, scale);
```

Resulting description of `rtype.Transform` (size 20, alignment 4):

| Field | `FieldType` | Offset | Size |
|---|---|---|---|
| `position` | `kVec2` | 0 | 8 |
| `rotation` | `kF32` | 8 | 4 |
| `scale` | `kVec2` | 12 | 8 |

### The serializer

```mermaid
flowchart LR
    bytes["Component bytes"] --> walk["Walk FieldInfo list"]
    info["ComponentInfo.fields"] --> walk
    walk --> binary["Binary writer<br/>network, saves"]
    walk --> luau["Luau table<br/>scripts"]
    walk --> text["Text / JSON<br/>scenes, debug"]
```

The serializer has one loop over fields and several **writers** (binary, Luau table, text). Each writer
knows how to write each `FieldType`. Adding a format means adding a writer; adding a component means
nothing.

### Network options per field

Replication wants fewer bytes than the in-memory form. Fields can carry options:

| Option | Example | Effect |
|---|---|---|
| Quantization | `position` as fixed-point `i16`, 1/8 pixel precision | 8 bytes → 4 bytes |
| Range | `rotation` in [0, 2π) on 10 bits | 4 bytes → ~1.3 bytes |
| Skip | client-only fields | not sent |

```c++
app.component<Transform>()
    .replicate(rtype::ecs::ReplicationMode::kEveryChange)
    .quantize("position", rtype::ecs::Quantize::fixedPoint(16, 1.F / 8.F));
```

### `kEntity` and `kAssetId` fields

Two field types are not copied as raw numbers:

- **`kEntity`**: written as the target's `NetworkId` (or "none"), and translated back to a local
  `Entity` on the receiving side.
- **`kAssetId`**: already stable across processes (derived from the asset path), written as is.
- **`kString`**: an `ecs::String` (`ecs/type/String.hpp`), a text held inline: a `u8` length and 255 bytes of UTF-8,
  256 bytes in all, so it is plain data with the same size on every compiler and needs no hook. A longer text is cut
  at a character boundary. On the wire it is the length then the bytes only, so a component with a string has a
  variable size (`isVariableSize()`): `computeWireSize()` gives its upper bound, and the snapshot writes such a
  component after its `u16` length (`snapshot::kVariableSize`).

- **Lists**: a `std::vector<T>` of trivially copyable values with a `FieldTypeOf<T>` mapping is a field, except
  for `std::vector<bool>`. Its `FieldInfo::list` tells how to reach its values
  (`ecs/type/FieldList.hpp`): they live outside the value and their number changes, so `offset` and `count` describe
  the vector itself and `type` its values. On the wire it is a `u16` count then the values, so its component has a
  variable size; `readFields()` then needs a live value, since a value that owns memory is not bytes. A count the
  received bytes cannot hold is refused before anything is allocated. A list of entities leaves out the ones the other
  side does not know, and loses an entity that is despawned instead of keeping a null one.
- **`kText`**: a `std::string` field is a list of bytes, read as one text of any length. It is for C++ components and
  for the call data of engine calls (a text longer than 255 bytes, such as a prompt): a component declared in Luau has
  no hooks and uses `kString`.

A type with a list has a size and offsets that depend on the standard library, so client and server compare a hash of
its name and fields instead of its layout hash (`computeManifestHash()`). A value that does not fit in a message is not
sent: messages hold 1000 bytes.

### Determinism

Field order comes from the layout rules in [Component registry](02-component-registry.md) (alignment,
then name), so two processes serialize the same component the same way. The `layoutHash` guards against
mismatches.

## Connections

- Uses: [Component registry](02-component-registry.md) (`FieldInfo`, layout).
- Used by: [Actors and scenes](18-actors-and-scenes.md), [Luau bridge](19-luau-bridge.md),
  [Replication](20-replication.md), debug tooling.

## Pitfalls

- **Forgetting a field in the macro.** It will not be replicated or visible to Luau. The macro can
  `static_assert` that the listed fields cover `sizeof(T)` (allowing for padding).
- **Pointers and containers in components.** `std::vector` or raw pointers cannot be reflected as
  `FieldType`s. Keep replicated components to plain fields and fixed arrays.
- **Endianness.** The binary writer fixes one byte order (little-endian), so mixed platforms agree.

## Open questions

- Per-field change masks to serialize only changed fields? (see [Change detection](08-change-detection.md))
