# Capsule file format

The text format of capsule files: what `format::readCapsule()` reads and `format::writeCapsule()` writes
(`src/rtype/plugins/capsule/format/CapsuleText.*`). What a capsule is and how it spawns is in
[`ecs/features/18-capsules.md`](ecs/features/18-capsules.md); the same format for game developers is the manual's
*Capsule file format* page (`manual/pages/capsule-format.md`). Why this format and not TOML:
[`benchmarks/capsule-format.md`](benchmarks/capsule-format.md).

```
[capsule format=1]

# The boss, with a turret on each side.
[node path="."]
game.Health = { value = 50, max = 50 }
rtype.Transform = {}

[node path="LeftTurret" instance="actors/turret.actor"]
rtype.Transform = { position = (-40, 0) }

[node path="LeftTurret/Muzzle"]
rtype.Transform = { position = (0, -12), rotation = 1.5708 }

[node path="Shadow" side=client]
rtype.SpriteRenderer = { texture = "sprites/boss_shadow.png", layer = -1 }
```

## Files

| Rule | |
|---|---|
| Extensions | `.capsule`, `.scene`, `.actor` (`format::kExtensions`): one format, the extension only tells a developer what the file is for |
| Place | anywhere under `<game>/assets/` |
| Name | the file's path in the assets, normalized (`core::asset::normalizePath()`: forward slashes, no `.` or `..`): `actors/boss.actor` |
| Id | `CapsuleId`, the `AssetId` of that path, the same in every process |
| Encoding | bytes; names are ASCII, strings may hold any UTF-8 |
| Version | `[capsule format=1]`, `format::kFormatVersion` |

## Lexical rules

The file is a stream of tokens. Blanks (spaces, tabs, line ends) separate tokens and are otherwise ignored, so a line
end has no meaning: a component may span several lines, and several may share one.

| Token | Rule |
|---|---|
| Comment | `#` to the end of the line, anywhere a blank may be (not inside a string) |
| Punctuation | `[` `]` `{` `}` `(` `)` `=` `,` |
| Name | a letter or `_`, then letters, digits, `_` and `.`: `rtype.Transform`, `side`, `true` |
| String | `"` ... `"` on one line. Escapes: `\n` (line end), `\t` (tab); any other `\c` stands for `c`, so `\"` and `\\` |
| Number | starts with a digit, `-` or `+`, then letters, digits, `.`, `-`, `+`: `40`, `-0.25`, `1e-5`, `0x1F`. What it means is checked against the field it fills |

Anything else (`.5`, `'text'`, `@`) is `unexpected character`.

## Grammar

```
file       = { section } ;
section    = "[" NAME { attribute } "]" { component } ;
attribute  = NAME "=" ( STRING | NAME | NUMBER ) ;
component  = NAME "=" "{" [ field { "," field } [ "," ] ] "}" ;
field      = NAME "=" value ;
value      = NUMBER | STRING | "true" | "false"
           | "(" [ value { "," value } [ "," ] ] ")"      (* a vector: numbers only *)
           | "[" [ value { "," value } [ "," ] ] "]" ;    (* an array or a list *)
```

A trailing comma is allowed. The section name is `capsule` or `node`; only a `node` section takes components.

## Sections

### `[capsule format=1]`

Optional. When present, it comes once and first. Its one attribute, `format`, must be the version this engine reads
(`1`). A file without it is read as the current version; `writeCapsule()` always writes it.

### `[node ...]`

One node: one entity when the capsule spawns. The nodes come in the order they spawn, the root first and every
parent before its children.

| Attribute | Value | Rule |
|---|---|---|
| `path` | string, required | `"."` (or `""`) for the root, which comes once and first. Otherwise the node's path from the root: `"LeftTurret"` is a child of the root, `"LeftTurret/Muzzle"` a child of `LeftTurret`. The part before the last `/` names the parent, which must already be in the file; the last part is the node's name: not empty, not `.` or `..`, and unique among its siblings |
| `instance` | string | The node spawns that capsule (its path in the assets) in its place: see [Instances](#instances) |
| `side` | `server`, `client` or `both` (a name or a string) | Where the node and its subtree are spawned; `server` when left out. Not on the root, which spawns on every side |

An attribute not in this table is an error. An attribute given twice keeps its last value.

### Components

Each `name = { ... }` after a `[node]` gives the node a component, by the name it was registered with (a C++
component, or one a script declared: files are read once every plugin and script is built). The fields are written
by the names reflection gives them (`RTYPE_ECS_COMPONENT`, or the script's declaration), in any order.

- A field left out keeps its starting value: the component's defaults, or on an instance node the instanced root's
  value.
- `{}` gives the component with all its starting values; it is also how a tag is added.
- A component given twice on a node, a field given twice, an unknown component or field is an error.
- `rtype.Parent` and `rtype.Children` cannot be given: the path makes the hierarchy.
- A component marked server-only or client-only is read on every side; the spawn leaves it out where it does not live.

### Instances

`[node path="LeftTurret" instance="actors/turret.actor"]` places a copy of `actors/turret.actor` there. The instanced
capsule's root *is* this node:

- The node's components are overrides. A component the instanced root has starts from the root's value, so only the
  changed fields are written. A component it does not have starts from its defaults and is added.
- The node's children in this file are added under the instanced root, after its own children.
- The instanced file is read first (the loader reads it when it meets the attribute), so its values are known. A
  capsule that is still missing when the file is read starts from the defaults, and its spawn fails, naming it.
- A capsule that contains itself through instances, at any depth, cannot spawn: the spawn names the chain.

## Values

What a value must look like depends on the type of the field it fills (`ecs::FieldType`).

| Field type | C++ | Written as | Rule |
|---|---|---|---|
| `bool` | `bool` | `true`, `false` | |
| `i8` ... `i64`, `u8` ... `u64` | integers | `42`, `-3`, `+7`, `0x2A` | a whole number, decimal or hexadecimal after `0x`, that fits the type (`-` only for signed ones) |
| `f32`, `f64` | `float`, `double` | `0.25`, `-40`, `2`, `1.5e3` | a decimal number, exponent allowed; finite; no hexadecimal |
| `vec2`, `vec3`, `vec4` | `glm::vec2/3/4` | `(x, y)`, `(x, y, z)`, `(x, y, z, w)` | exactly that many numbers |
| asset id | `core::asset::AssetId` | `"sprites/ship.png"` or `0x9e3779b97f4a7c15` | a path, normalized and hashed into the id (the path is remembered, to write it back), or the raw 64-bit id for an asset with no path |
| string | `ecs::String` | `"Player one"` | at most 255 bytes |
| text | `std::string` | `"any length"` | |
| fixed array | `std::array<T, N>` | `[a, b, c]` | exactly `N` values of the element's type (`[(0, 0), (1, 0)]` for vectors) |
| list | `std::vector<T>` | `[a, b, c]`, `[]` | any number of values, replacing the starting list |
| entity | `ecs::Entity` | | not yet: `cannot be set in a capsule file yet` |

A vector holds numbers only. A value of the wrong kind names what the field takes: `field 'position' of
'rtype.Transform' takes a vector of 2, not 3`.

## Errors

Reading stops at the first mistake, with an exception (`exceptions::CapsuleException`) whose message starts with the
file, the line and the column, from 1:

```
actors/boss.actor:7:31: field 'value' of 'game.Health' takes a whole number from -2147483648 to 2147483647, not 5O
```

| Message | Cause |
|---|---|
| `unexpected character 'c'` | a character no token starts with |
| `this string has no closing quote` | a string that reaches the line end |
| `expected '[' to start a section, found ...` | text outside a section, or a component after `[capsule]` |
| `unknown section 'x': expected 'capsule' or 'node'` | |
| `the [capsule] section comes once, first` | |
| `format 2 is not known: this engine reads format 1` | a file written by another version |
| `unknown attribute 'x': [node] takes 'path', 'instance' and 'side'` | the same for `[capsule]`, which takes `format` |
| `a [node] needs a path: path="." for the root, path="Boss/Turret" below it` | |
| `the first node is the root: path="."` / `the root (path=".") comes once, first` | |
| `'A/B' has no parent node 'A' above it` | a child before its parent, or a typo in the parent's name |
| `... already has a child named 'B'` | two siblings with one name |
| `unknown side 'x': expected server, client or both` / `the root is spawned on every side: it takes no 'side'` | |
| `unknown component 'x'` | a name no plugin or script registered |
| `component 'x' is set twice on this node` / `field 'f' is set twice` | |
| `component 'x' has no field 'f'` | |
| `field 'f' of 'x' takes ..., not ...` | a value of the wrong kind, a number out of range, a vector or array of the wrong size, a string over 255 bytes |

At start-up a file that cannot be read stops the game. Read again by hot reload, it keeps its previous version, and the
reload is reported as failed with this message.

## Writing

`writeCapsule()` writes a capsule so that reading the text back gives the same capsule. An editor can therefore load a
file, change it in memory and save it.

- `[capsule format=1]`, then each node in the capsule's order, separated by a blank line.
- A node's header gives its full `path`, its `instance` and its `side` (left out when `server`).
- Its components come in the order they were read or added, one per line, `name = { field = value, ... }`.
- **Only the fields that differ from the starting value** are written: from the defaults, or from the instanced root's
  value on an instance node. `{}` when none differs. Entity fields are not written.
- Floats are written in the shortest form that reads back exactly. Plain decimals (`-40`, `0.1`) are used for
  magnitudes from 1e-4 to 1e15, an exponent otherwise; `-0` is written `0`.
- Integers are written in decimal. Booleans are `true` / `false`. Vectors are `(a, b)`, arrays and lists `[a, b]`.
- An asset id is written as the path it was read from (`Capsules::findAssetPath()`), or as `0x` and 16 hexadecimal
  digits for an id with no known path.
- Strings are quoted, with `"`, `\`, line ends and tabs escaped.
- Comments and blank lines are not kept: a file saved by the engine loses them.

## Versions

`format=1` is the only version. A change that existing files cannot be read with, or that changes what they mean,
raises `kFormatVersion`. The reader refuses a version it does not know rather than guess. A change that only adds
something (a new attribute, a new value form) keeps the version.

## Limits

- Entity fields cannot be set yet (they would name a node of the same capsule).
- `Capsule::addNode()` checks a name against every node, so a level of thousands of nodes under one parent reads in
  quadratic time (about 40 ms for 5,000 nodes, see the benchmark).
