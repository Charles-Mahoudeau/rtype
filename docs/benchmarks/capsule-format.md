# Capsule file format: our own text or TOML (November 2026)

**Context:** capsules (actors, levels, any entity tree, see [`ecs/features/18-capsules.md`](../ecs/features/18-capsules.md))
are described by `.capsule`, `.scene` and `.actor` files under a game's assets. They are read when the game starts and
again when a file is saved (hot reload), and written back by the engine so an editor can save what it reads. The
format we use, specified in [`CapsuleFormat.md`](../CapsuleFormat.md), looks like TOML but is not TOML. The question:
should we switch to real TOML and a TOML library, or keep our own reader and writer?

What the format must do:

- **Keep the file's order.** A node's components and fields come back in the order they were written, so an editor
  that saves a file changes only what was edited, and diffs stay small.
- **Map onto the reflection.** Every field type of a component (integers, floats, vectors, asset ids, strings, lists,
  fixed arrays) has one way to be written, and a mistake is reported with the file, line, column, component and field.
- **Hold 64-bit asset ids.** An asset id is a 64-bit hash, written as its path, or as a number for an id with no path.
- **Read fast enough for hot reload,** and add as little as possible to the build (one plugin library, three platforms,
  three compilers).

> **Decision: we keep our own format, with no dependency.**
> - The best TOML library for us, tomlc17, reads faster than ours, keeps the file's order and allows components over
>   several lines.
> - What TOML would cost in features can be worked around.
> - So the choice is not about speed or features but about dependencies. Our files are read and written by the engine,
>   and its editor goes through it, so TOML's advantages (a standard syntax, tools in other languages) buy little.
> - The cost would be one more package to build and cache in CI on three platforms, and a young library whose MSVC
>   build we have not tested.

---

## 1. The same capsule in both

Ours ([`CapsuleFormat.md`](../CapsuleFormat.md)):

```
[capsule format=1]

[node path="."]
game.Health = { value = 50 }

[node path="LeftTurret" instance="actors/turret.actor"]
rtype.Transform = { position = (-40, 0) }

[node path="Background" side=client]
rtype.SpriteRenderer = { texture = "sprites/bg.png", layer = -10 }
```

The closest TOML:

```toml
[capsule]
format = 1

[[node]]
path = "."
game.Health = { value = 50 }

[[node]]
path = "LeftTurret"
instance = "actors/turret.actor"
rtype.Transform = { position = [-40.0, 0.0] }

[[node]]
path = "Background"
side = "client"
rtype.SpriteRenderer = { texture = "sprites/bg.png", layer = -10 }
```

Our section headers carry attributes (`[node path="." side=client]`), and `(x, y)` vectors and bare words such as
`client` are not TOML values: a TOML library cannot read our files as they are. In TOML, `game.Health = {...}` is a
table `game` holding `Health`. A converter can join those parts back into the component's name: the quotes the
first draft of this comparison assumed (`"game.Health" = {...}`) are not needed.

---

## 2. The libraries

| Library | Language | License | Package | Notes |
|---|---|---|---|---|
| **toml++** 3.4.0 | C++17, header-only | MIT | `toml++` | the best known; sorts a table's keys |
| **toml11** 4.4.0 | C++11 | MIT | `toml11` | `ordered_type_config` keeps the order and the comments; TOML 1.1 on request |
| **tomlc17** 2026.08.21 | C17, one source file | MIT | `tomlc17` | TOML 1.1, keeps the order, read only; first released in 2025 |

---

## 3. Timed benchmark

**Setup.**
- Intel Core Ultra 7 155H, Linux, clang 23, release build.
- The engine's `plugin-capsule` as built.
- Generated capsules: a root, then nodes that each carry a transform (two `vec2` and a float) and a health (two
  `i32`). Every third node also has a sprite (an asset path, an `i32`, a `vec4`), every seventh is a client node, and
  every tenth instances `actors/turret.actor` with a transform override.

**What is timed.**
- *Ours*: `format::readCapsule()`.
- *A TOML library*: its parse, then one conversion, the same template for the three libraries, into the same
  `Capsule`. The conversion goes through the same reflection: `Capsule::addNode()` and `ComponentValues::set()`,
  overrides starting from the instanced root, asset paths remembered.
- Each library gives the same capsule as ours, which the benchmark checks node by node before timing.
- The conversion handles only the field types the benchmark uses and gives short messages, so it is a lower bound for a
  real TOML reader.
- Each figure is the median of many runs (20,000 for the actor, 2,000 for the level, 50 for the big level), over three
  runs that agree within 15%.

| Capsule | Ours | toml++ | toml11 (ordered) | tomlc17 |
|---|---|---|---|---|
| actor, 8 nodes | 23 µs | 45 µs (1.95×) | 470 µs (20×) | **19 µs (0.83×)** |
| level, 200 nodes | 0.9 ms | 1.3 ms (1.4×) | 12.7 ms (14×) | **0.58 ms (0.65×)** |
| big level, 5,000 nodes | 41 ms | 56 ms (1.35×) | 350 ms (8.5×) | **37 ms (0.9×)** |
| Keeps the file's order | yes | no | yes | yes |

**What the numbers say.**
- tomlc17 reads faster than our reader. Ours first splits the whole file into tokens, each holding its own
  `std::string`, and those allocations dominate; tokens that point into the text would close the gap if it ever
  matters.
- toml++ spends longer parsing into its own tree than our reader spends building the capsule. toml11 is an order of
  magnitude slower.
- None of this matters in practice: every option but toml11 reads a level in under a millisecond, well within a hot
  reload.
- At 5,000 nodes under one parent everything slows down. `Capsule::addNode()` checks a new name against every node,
  which is quadratic in a flat level, and every pipeline calls it. That is a fix to make in `Capsule` (an index of
  children by name), whatever the format.

---

## 4. Features

| Point | Ours | toml++ | toml11 | tomlc17 |
|---|---|---|---|---|
| Order of a node's components | kept | lost (sorted) | kept (ordered config) | kept |
| A component over several lines, trailing comma | yes | no (TOML 1.0) | with `spec::v(1, 1, 0)` | yes (TOML 1.1) |
| An asset id of 2^63 or more as a number (`0x9e3779b97f4a7c15`) | yes | refused | refused | refused |
| Column of a syntax error | yes | yes | yes | line only |
| Writes TOML | (our writer) | yes | yes | no |

TOML integers are signed 64-bit, so every library refuses half of the 64-bit asset ids written as numbers. Only an id
with no known path is written that way (one set in code). With TOML it would have to be a string (`"0x9e37..."`).

---

## 5. Build cost

| | Ours | With a TOML library |
|---|---|---|
| Code | `format/CapsuleText.cpp`, 1,000 lines: lexer and grammar (about 500), value conversion (about 200), writer and messages | the library, plus a converter: the value conversion and the messages stay, the writer stays (rewritten for TOML) |
| Dependency | none | a package to build and cache in CI on Linux, macOS and Windows |
| toml++ | | +178 KB in a stripped shared library that only parses and prints; 2.6 s to compile a file that includes it; `-Wdeprecated-literal-operator` warnings with recent clang |
| tomlc17 | | a 59 KB static library from one C file; no column in syntax errors; MSVC build not tested here |

---

## 6. Decision

**We keep our own format and its reader and writer, with no dependency.**

- **What TOML would bring:** a standard syntax, highlighting and linting in every editor, and readers in other
  languages for tools outside the engine. Our files are read and written by the engine, and its editor goes through
  it, so these buy little today.
- **What it would cost:** a new package on three platforms, where package caches have already been a source of CI
  trouble. tomlc17, the only library that is both fast and order-preserving, is young.
- **Speed and features do not decide it.** tomlc17 matches or beats us, and TOML's limits can be worked around:
  dotted names can be joined back into component names, and large ids written as strings.

Revisit this if a tool outside the engine (a level generator in Python, a web editor) needs to read or write capsule
files: TOML with tomlc17 would then be the choice.

Follow-ups:
- `Capsule::addNode()` and `findNode()` scan every node. An index of children by name makes a flat level linear.
  This is independent of the format.
- Our lexer could keep tokens as views into the text, to read as fast as tomlc17 if loading time ever matters.
- A syntax-highlighting definition for editors (a TextMate grammar) gives back the most visible advantage of TOML.
