# 18 · Capsules

## What it is

A **capsule** is an entity described once and spawned by copy: its components, and its child entities, any of which
can be another capsule. An enemy, a bullet, a boss with its turrets and a whole level are all capsules: there is one
behaviour, not one for templates and another for levels. Other engines split it in two (prefabs and scenes); Godot
does not, and neither do we.

A capsule's file says what the developer means by its extension: `.actor` for a thing that moves, `.scene` for a level,
`.capsule` for anything else. The content and the behaviour are the same; the extension is only there to organize a
game's files.

## Why we need it

- **Convenience (R2):** spawning `actors/bydo.actor` instead of adding five components by hand in every system that
  creates an enemy.
- **R4, data-driven content:** designers describe enemies and levels in files, which an editor can read and write,
  without touching systems; a saved file is reloaded while the game runs.
- **R3 (network):** every spawned root carries a `CapsuleInstance` (capsule id, version), so the server can later tell a
  client "spawn this capsule here" in a few bytes (see [Replication](20-replication.md)).

## How others do it

| Engine | Templates and levels |
|---|---|
| Godot | One thing: a scene is a tree of nodes, a node can instance another scene; `.tscn` text files |
| Unity | Prefabs (editor assets) and scenes, two concepts; DOTS bakes them to entities |
| Bevy | Bundles in code, `DynamicScene` files for sets of entities |
| flecs | Prefabs are entities, instances inherit their components (`IsA`) |

## Our choice

- **A plugin, not an ECS feature:** capsules only use the ECS's public API (spawn, add a component by id and bytes,
  `Commands`, `setParent`), so they live in `plugin-capsule` (`rtype.Capsule`), as `bevy_scene` lives outside
  `bevy_ecs`.
- **One behaviour, two steps.** A file is read once into a capsule: each node's components as bytes ready to copy. A
  spawn then only allocates entities and copies bytes, so spawning a bullet hundreds of times never parses text.
- **A flat list of nodes**, the file's structure one to one: name, parent, side, the capsule it instances if any, and
  its components. The hierarchy is made at spawn, with `Parent` and `Children`.
- **Instances by copy**, no inheritance: changing a capsule changes what is spawned after, and what was spawned before
  when it is reloaded.

## How it works

### Adding the plugin

A game names it in its `project.luau`, and the `Engine` loads it from its library (see
[Plugins](../../Plugins.md#loading)):

```lua
engine.settings.plugins = { "rtype.Capsule" }
```

A program on a bare `App` (tests, examples) adds `rtype::plugins::capsule::CapsulePlugin{}`. It inserts the `Capsules`
resource (`"rtype.Capsules"`), the `LoadedCapsule` resource (`"rtype.LoadedCapsule"`), registers the `CapsuleInstance`
component and the engine calls. It requires no plugin and reads no settings.

### A capsule

A capsule is named by its path ("actors/bydo.actor"); its id is the `AssetId` of that path, the same in every process.
Its nodes form a flat list, a parent before its children, node 0 the root:

| Node field | Meaning |
|---|---|
| name | Its name among its parent's children (not empty, no `/`, unique there); a node's path is "Boss/LeftTurret" |
| parent | Its parent node |
| side | Where it is spawned: the server by default, the client, or both (`ecs::runsIn()`, as for systems). The root is spawned everywhere |
| instance | Another capsule it spawns instead of a plain entity: that capsule's root takes this node's components over its own, and gets this node's children too |
| components | `ComponentValues`: one live value per component (copied and destroyed with its type's hooks), each with the component's presence |

From C++, `addCapsule()` defines a capsule and returns its root's `NodeConfig`:

```c++
rtype::plugins::capsule::addCapsule(app, "actors/turret.actor").with(Transform{}).with(Health{.value = 5});
rtype::plugins::capsule::NodeConfig boss = rtype::plugins::capsule::addCapsule(app, "actors/boss.actor");
boss.with(Transform{}).with(Health{.value = 50});
boss.child("LeftTurret").instance("actors/turret.actor").with(Transform{.position = {-40.F, 0.F}});
boss.child("RightTurret").instance("actors/turret.actor").with(Transform{.position = {40.F, 0.F}});

rtype::plugins::capsule::NodeConfig level = rtype::plugins::capsule::addCapsule(app, "scenes/level1.scene");
level.child("Boss").instance("actors/boss.actor").with(Transform{.position = {600.F, 0.F}});
level.child("Background").onSide(rtype::ecs::Side::kClient).with(SpriteRenderer{/* ... */});
```

An instance is looked up when the capsule is spawned, so a capsule can instance one defined, or redefined, later. Files
describe capsules the same way (the capsule file format).

| Rule | Behaviour |
|---|---|
| Names | One capsule per path; two paths with the same id are refused (`CapsuleException`) |
| Values | Any component (all are copyable); setting one again replaces it. `Parent` and `Children` are refused: the nodes set the hierarchy |
| Hot reload | `Capsules::define(path)` again empties the capsule and bumps its version, keeping its id, so what instances it stays valid |

### Spawning

```c++
// in a system: reserved at once, alive when the system's commands are applied
void spawnWave(rtype::ecs::Commands& commands, rtype::ecs::Res<rtype::plugins::capsule::Capsules> capsules) {
    rtype::plugins::capsule::spawnCapsule(commands, *capsules, "actors/bydo.actor")
        .add(Transform{.position = {820.F, 140.F}});   // replaces the capsule's Transform for this entity
}

// outside systems (tests, tools): at once, from the world's Capsules
const rtype::ecs::Entity boss = rtype::plugins::capsule::spawnCapsule(world, "actors/boss.actor");
```

```mermaid
flowchart LR
    file["actors/boss.actor"] -->|read once| capsule["Capsule<br/>nodes with component bytes"]
    capsule -->|copy per spawn| world["World<br/>one entity per node, parented"]
```

One walk over the nodes, shared by `Commands` and the `World`: a node of the other side is skipped with its subtree; a
node is a new entity, or the root its instance spawns; its kept values are added (presence: a client leaves out
`kServerOnly` components, a dedicated server `kClientOnly` ones); it is parented to its parent's entity. The root gets a
`CapsuleInstance { capsule, version, nested }`. A check pass runs first (every instanced capsule exists, none contains
itself), so a spawn creates everything or nothing.

A level is a capsule whose root has no `Transform`: the `TransformPlugin` treats its children as roots, so they stay in
world space. A capsule that moves as one gives its root a `Transform`. Despawning a root despawns the capsule: its
descendants go with it.

### Loading: switching levels

`loadCapsule()` (and the `rtype.capsule.load` call) switches the level: it despawns the root the `LoadedCapsule`
resource names, spawns the new capsule and remembers it. Entities outside the old capsule's tree (the player, a game
manager) stay.

### Reloading

| Function | Behaviour |
|---|---|
| `reloadCapsule(world, root)` | Despawns the root's children, adds the capsule's current root values again, spawns its children again and updates `CapsuleInstance.version`. The root entity and its other components stay |
| `reloadCapsule(commands, capsules, root, instance, children)` | The same, recorded, from a system |
| `reloadInstances(world, id)` | Reloads everything spawned from a capsule; a nested root (`nested`) is reloaded through the outermost capsule it belongs to, so the outer node's values stay on it |

### From scripts

Scripts describe nothing: files do. A system spawns, loads and reloads through three engine calls, listed in its
`calls`; each records into the system's own commands and adds what it reads and writes to the system's access.

| Call | Arguments | Result |
|---|---|---|
| `rtype.capsule.spawn` | `capsule` (a path) | the root, reserved at once |
| `rtype.capsule.load` | `capsule` (a path) | the new root; the capsule loaded before is despawned |
| `rtype.capsule.reload` | `root` | `true` if it was a capsule's root |

```luau
engine.ecs.system("game.Waves", {
    phase = "Update",
    calls = { "rtype.capsule.spawn", "rtype.capsule.load" },
    run = function(ctx)
        local bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
        ctx.commands:add(bydo, "rtype.Transform", { position = { x = 820, y = 140 } })
        -- when the boss is defeated:
        ctx.call("rtype.capsule.load", { capsule = "scenes/level2.scene" })
    end,
})
```

An override added after a spawn replaces the whole component (its defaults with the fields given), as in C++.

## Connections

- Uses: [Component registry](02-component-registry.md) (ids, presence), [Commands](09-commands.md),
  [Hooks](16-hooks-and-observers.md) (`copy`), [Hierarchy](17-hierarchy.md), [Reflection](15-reflection-and-serialization.md)
  (the file format).
- Used by: [Luau bridge](19-luau-bridge.md) (the calls), [Replication](20-replication.md) (spawn by capsule id, later).
- Code: `src/rtype/plugins/capsule/`; example `examples/plugins/capsule/`; manual page `manual/pages/capsules.md`.

## Pitfalls

- **A node on the wrong side.** A node is server-side by default: on a client, a camera or a background must be marked
  `client` (or `both`), or it is not spawned there.
- **Reloading a root the game added to.** A reload despawns all the root's children, including ones the game parented
  to it, and keeps the root's other components, even ones the new version no longer sets.
- **Editing a capsule after spawning.** Instances are copies: they change only when reloaded.

## Open questions

- Remapping `Entity` fields that point inside the tree (a turret aiming at a node of its own capsule). (Later)
- Saving part of a running world back to a capsule file, for an editor. (Later)
