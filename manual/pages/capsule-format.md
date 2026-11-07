# Capsule file format

A capsule file describes an entity tree: an enemy, a boss with its turrets, a whole level. This page is the complete
reference of what you can write in one. What capsules are, and how a system spawns them, is on the
[Capsules](capsules.md) page.

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

- Put capsule files anywhere under `assets/`, with the extension `.capsule`, `.scene` or `.actor`. The three are the same
  format: pick the one that tells you what the file is for.
- A capsule's name is its path from `assets/`: `actors/boss.actor`. You use that name to spawn it and to instance it
  in another file.
- The engine reads every capsule file when the game starts, after your scripts, so a file can use the components your
  scripts declare. With [hot reload](hot-reload.md), it reads a file again when you save it. See
  [Reading and writing capsule files](capsule-files.md).

## Writing a file

A file is a list of sections, each starting with a line in square brackets.

- **Line ends do not matter.** A component can span several lines, and a trailing comma is fine.
- **`#` starts a comment**, to the end of the line.
- **Names** (components, fields, attributes) are letters, digits, `_` and `.`.
- **Strings** are in double quotes, on one line, with `\"` for a quote, `\\` for a backslash, `\n` for a line end and
  `\t` for a tab.

```
[node path="Boss"]
game.Weapon = {
	damage = 12,
	cooldown = 0.4,   # seconds
}
```

### `[capsule format=1]`

Optional. If you write it, it comes first, once. It says the file uses version 1 of the format, the one this engine
reads; a file of another version is refused with a message saying so.

### `[node ...]`

Each `[node]` is one entity. The root comes first, and a parent always comes before its children.

| Attribute | Value | Rule |
|---|---|---|
| `path` | a string, required | `"."` is the root. Any other path names the node from the root: `"LeftTurret"` is a child of the root, `"LeftTurret/Muzzle"` a child of `LeftTurret`. The part before the last `/` must be a node written earlier; the last part is the node's name, unique among its siblings and not `.` or `..` |
| `instance` | a string | Spawns another capsule in this node's place, by its path from `assets/`. See [Instances](#instances) |
| `side` | `server`, `client` or `both` | Where the node and its children are spawned. `server` when you leave it out. The root has no `side`: it spawns everywhere |

Nodes are the server's by default, and the server's entities reach the players by replication. Use `side=client` for
what only players see (a shadow, a particle effect), so it is not sent over the network.

### Components

After a `[node]` line, each component is its name, `=`, and its fields between braces:

```
rtype.Transform = { position = (120, 80), rotation = 0.5 }
game.Health = { value = 3 }
game.Boss = {}
```

- The name is the one a script declared (`game.Health`) or the engine's (`rtype.Transform`).
- Fields you leave out keep the component's defaults; `{}` keeps them all, and adds a component that has no fields.
- A component appears once per node, and a field once per component.
- `rtype.Parent` and `rtype.Children` cannot be written: the paths make the tree.

### Instances

```
[node path="LeftTurret" instance="actors/turret.actor"]
rtype.Transform = { position = (-40, 0) }

[node path="LeftTurret/Light" side=client]
rtype.SpriteRenderer = { texture = "sprites/light.png" }
```

`instance` puts a whole copy of `actors/turret.actor` at this node: its root becomes this node, and its children come
with it.

- **Write only what you change.** A component the turret's root already has starts from the turret's values, so
  `position` above changes and every other field stays the turret's. A component the turret does not have is added,
  from its defaults.
- **Children you write** under an instance node (`LeftTurret/Light`) are added to the turret, after its own children.
- **Instances nest.** The turret file can itself instance other capsules. A capsule that contains itself, at any depth,
  cannot spawn, and the error names the chain.
- When you save `actors/turret.actor`, every file that instances it is read again too.

## Values

How you write a value depends on the type of the field (see [Components](components.md#field-types)).

| Field type | Write | Rule |
|---|---|---|
| `bool` | `true`, `false` | |
| `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64` | `42`, `-3`, `+7`, `0x2A` | a whole number that fits the type; hexadecimal after `0x`; no `-` for the `u` types |
| `f32`, `f64` | `0.25`, `-40`, `2`, `1.5e3` | a number; write `0.5`, not `.5`; no `inf` or `nan` |
| `vec2`, `vec3`, `vec4` | `(x, y)`, `(x, y, z)`, `(x, y, z, w)` | parentheses, exactly that many numbers |
| `assetId` | `"sprites/ship.png"` | the asset's path from `assets/` |
| `string` | `"Player one"` | at most 255 bytes |
| a list | `[1, 2, 3]`, `[]` | any number of values, replacing the whole list |
| `entity` | | cannot be written in a file yet |

## Errors

The engine stops at the first mistake in a file. The message starts with the file, the line and the column:

```
actors/boss.actor:7:31: field 'value' of 'game.Health' takes a whole number from -2147483648 to 2147483647, not 5O
```

When the game starts, a broken file stops it. Saved while the game runs, a broken file keeps its previous version, and
the hot reload line in the log carries the message.

| Message | What to fix |
|---|---|
| `unexpected character 'c'` | a character that has no place here, such as `.5` (write `0.5`) or single quotes |
| `this string has no closing quote` | a string that runs to the end of the line |
| `expected '[' to start a section, found ...` | text before the first section, or a component under `[capsule]` |
| `unknown section 'x': expected 'capsule' or 'node'` | a typo in a section name |
| `format 2 is not known: this engine reads format 1` | a file made for another version of the engine |
| `unknown attribute 'x': [node] takes 'path', 'instance' and 'side'` | a typo in an attribute |
| `a [node] needs a path: ...` | a `[node]` without `path` |
| `the first node is the root: path="."` | the file does not start with the root |
| `'A/B' has no parent node 'A' above it` | a child written before its parent, or a typo in the parent's name |
| `... already has a child named 'B'` | two siblings with the same name |
| `unknown side 'x': expected server, client or both` | |
| `the root is spawned on every side: it takes no 'side'` | `side` on the root |
| `unknown component 'x'` | a typo, a component no script declares, or one of a plugin `project.luau` does not list |
| `component 'x' has no field 'f'` | a typo in a field name |
| `component 'x' is set twice on this node` / `field 'f' is set twice` | |
| `field 'f' of 'x' takes ..., not ...` | a value of the wrong kind: a number out of range, a vector of the wrong size, a string too long |

## Reading and writing

When the engine reads your files, what saving one does to a running game, and what a file looks like once the engine
writes it back (comments dropped, only the fields that differ): see
[Reading and writing capsule files](capsule-files.md).

## See also

- [Reading and writing capsule files](capsule-files.md): when files are read, hot reload, and how the engine writes them.
- [Capsules](capsules.md): spawning, switching levels, reloading from a system.
- [Components](components.md): declaring the components a file uses.
- [Hot reload](hot-reload.md): what happens when you save a file.
