# Components

A component is a named bag of fields attached to an entity. You declare yours with `engine.ecs.component`, at load
time, in the same way on the server and on every client.

```luau
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" },
	defaults = { player = 0 },
	replication = "everyChange",
})

engine.ecs.component("game.MisfireTimer", { fields = { remaining = "f32" }, presence = "serverOnly" })
```

## The declaration

| Option | Meaning |
|---|---|
| `fields` | **Required.** A table of field name to type name. An empty table makes a tag. |
| `defaults` | The starting value of some fields; a field not listed starts at zero. It can only name declared fields. |
| `replication` | `"none"` (default), `"everyChange"` (sent whenever it changes) or `"spawnOnly"` (sent once, when the entity appears). |
| `presence` | `"both"` (default), `"serverOnly"` or `"clientOnly"`: which side the component exists on. |

Any other option is an error. Names are conventionally `game.Name`; the `rtype.` and `net.` prefixes belong to the
engine.

## Field types

| Type | In a script |
|---|---|
| `bool` | `true` / `false` |
| `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64` | a number |
| `f32`, `f64` | a number |
| `vec2`, `vec3`, `vec4` | a table `{ x = ..., y = ... }` (`z`, `w` for the larger ones) |
| `entity` | an entity |
| `assetId` | the id of an asset; a path is accepted where a value is written |
| `string` | a string (limited to 255 bytes) |

The layout of a component is the same on every machine. When a client connects, the server checks that both sides
declared the same components, and refuses a client that differs, naming the component.

## Engine components

The engine's own components are declared by its plugins and used by name, with the same views as yours:
`rtype.Name` (`value`, a text: what the editor shows an entity as, given by a [capsule](capsules.md) to each entity it
spawns), `rtype.Transform`, `rtype.Camera`, `rtype.SpriteRenderer`, `rtype.TextRenderer`, `rtype.ShapeRenderer`,
`rtype.AudioListener`, `rtype.Collider`, `rtype.RigidBody`, `rtype.Velocity`, and the network ones `net.Owner` and
`net.PlayerInput`. Fields are written the same way (`size = { x = 48, y = 48 }`). Enum-like fields are numbers; for
example the physics `shape` is `0` box, `1` circle, `2` capsule. What exists depends on the plugins the program and
`project.luau` add: a server has no sprites.

## Reading and writing

A system receives a *view* of a component for each entity it matches (see [Systems](systems.md)). Fields read and write
like table fields: `ship.position`, `ship.player = 1`, `transform.position = { x = 4, y = 5 }`. A view is valid only
during the call to `run`: do not keep it. Writing a read-only view is an error. An entity is a value you can keep
(`entity.bits` identifies it, for example as a table key), and it stays valid across frames.

## Where components live

The server is authoritative. A component with `replication` other than `"none"` is sent from the server to the
clients, and **a system running on the client may not write it** (nor a `serverOnly` one); this is checked when the
system is declared and when a command is recorded. Put client-only presentation data in `presence = "clientOnly"`
components. See [Networking](networking.md).
