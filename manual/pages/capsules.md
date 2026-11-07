# Capsules

A capsule is an entity your game describes once and spawns as often as it wants: its components, and its child
entities, any of which can be another capsule. An enemy, a bullet, a boss with its turrets and a whole level are all
capsules. Name the file after what it is for: `.actor` for something that moves, `.scene` for a level, `.capsule` for
anything else. The engine treats them the same way.

Add the plugin in `project.luau`:

```luau
engine.settings.plugins = { "rtype.Capsule" }
```

## What a capsule holds

A capsule is a tree of nodes. Each node becomes one entity when the capsule is spawned:

- its **components**, with their values;
- its **children**, parented to it, so a turret follows its boss;
- its **side**: the server by default, `client` for what only players see (a background, a camera), or `both`;
- optionally, **another capsule** it spawns instead of a plain entity: a boss's turret node spawns
  `actors/turret.actor`, and the node's own components replace the turret's (its position on the boss).

A level is a capsule whose root has no `rtype.Transform`: its children stay where the level put them. A capsule that
should move as one gives its root a `rtype.Transform`.

## Capsule files

Put your capsules anywhere under `assets/`. The engine reads every `.capsule`, `.scene` and `.actor` file when the game
starts, after your scripts, so a file can use the components they declare. A capsule's name is its path from
`assets/`, for example `actors/boss.actor`.

```
[capsule format=1]

# The boss, with a turret on each side.
[node path="."]
game.Health = { value = 50 }
rtype.Transform = {}

[node path="LeftTurret" instance="actors/turret.actor"]
rtype.Transform = { position = (-40, 0) }

[node path="RightTurret" instance="actors/turret.actor"]
rtype.Transform = { position = (40, 0) }

[node path="Shadow" side=client]
rtype.SpriteRenderer = { texture = "sprites/boss_shadow.png", layer = -1 }
```

Each `[node]` is one entity: `path="."` is the root, `instance` places another capsule there (write only the fields
you change), and `side=client` keeps a node on the players' machines. The
[Capsule file format](capsule-format.md) page has the whole format: every attribute, how to write each field type,
instances and the error messages.

With [hot reload](hot-reload.md) on, saving a file respawns what came from it in place, and a file saved with a
mistake keeps its previous version. [Reading and writing capsule files](capsule-files.md) says when the engine reads
your files, what saving one does to a running game, and what a file looks like once the engine writes it back.

## Names

Each entity a capsule spawns gets a `rtype.Name`: the name of its node (`LeftTurret`), or for the root, the file's name
without its folder and extension (`boss` for `actors/boss.actor`). A node that gives a `rtype.Name` of its own keeps it.
The [editor](editor.md) shows entities by these names; a script can read them like any component.

## Spawning, switching levels, reloading

The `engine.capsule` forms do each of these in one line, from a system's `run`. Each makes an engine call, so the
system lists that call in its `calls`:

```luau
engine.ecs.system("game.Waves", {
	phase = "Update",
	query = {},
	calls = { "rtype.capsule.spawn", "rtype.capsule.load" },
	run = function(ctx)
		local bydo = engine.capsule.spawn(ctx, "actors/bydo.actor", {
			["rtype.Transform"] = { position = { x = 820, y = 140 } },
		})

		if bossDefeated then
			engine.capsule.load(ctx, "scenes/level2.scene")
		end
	end,
})
```

| Form | Needs in `calls` | Returns | What it does |
|---|---|---|---|
| `engine.capsule.spawn(ctx, path, overrides)` | `rtype.capsule.spawn` | the root entity | Spawns a copy of the capsule |
| `engine.capsule.load(ctx, path, overrides)` | `rtype.capsule.load` | the root entity | Switches levels: the capsule loaded before is despawned, then this one is spawned |
| `engine.capsule.reload(ctx, root)` | `rtype.capsule.reload` | `true` if `root` was a capsule's root, `false` otherwise | Spawns the capsule's current version again into the same root |

`path` is the capsule file's path in `assets/`. `overrides` is optional: a table of component name to fields, written
over the capsule root's values. **Only the fields you name change**: in the example the bydo keeps the rotation, the
scale and every other component its file gives it. A component the root does not have is not added by an override;
add it with `ctx.commands:add`. To change a child, find it with a query once it is spawned.

- The root is ready to use at once, to keep in a table or pass to other commands.
- The entities appear when the system's commands are applied, at the end of its phase, like `ctx.commands:spawn`.
- Despawning the root (`ctx.commands:despawn(root)`) removes the whole capsule.
- `load` only removes the capsule loaded before: your player and anything else spawned outside it stay.
- A reload despawns the root's children, including ones you attached to it, and keeps the root's other components.

### The calls underneath

The forms are thin: `engine.capsule.spawn(ctx, path, overrides)` is the call followed by one
[`ctx.commands:patch`](systems.md#commands) per override. You can make the calls yourself:

| Call | Arguments | Result |
|---|---|---|
| `rtype.capsule.spawn` | `capsule` (a path) | the root entity |
| `rtype.capsule.load` | `capsule` (a path) | the root entity |
| `rtype.capsule.reload` | `root` (an entity) | `true` if it was a capsule's root |

```luau
local bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
ctx.commands:patch(bydo, "rtype.Transform", { position = { x = 820, y = 140 } })
```

## Sides

Each node is spawned on its side only, so one capsule loads on the server and on every client without duplicates: the
server's entities reach the players by replication, and the client nodes stay local. Components marked server-only
are left out on clients, and client-only ones on a dedicated server.

## Errors

A call on a path no capsule has, or a capsule that contains itself through its instances, is a Luau error naming the
call. Nothing is spawned then. The forms start their errors with their name:

| Message | Cause |
|---|---|
| `engine.capsule.spawn: expected the system's ctx first` | The first argument is not the `ctx` of `run` |
| `engine.capsule.spawn: expected the capsule's path second ("actors/bydo.actor")` | The path is missing, empty or not a string |
| `engine.capsule.spawn: overrides: 'rtype.Transform' must be a table of fields` | An override is not a table |
| `engine.capsule.spawn: overrides: ...` followed by the field's error | An override names a field the component does not have, or a value that does not fit; the capsule is spawned without it |
| `engine.capsule.spawn: ...` followed by the call's error | An unknown capsule, or the call missing from the system's `calls` |

`engine.capsule.load` and `engine.capsule.reload` give the same messages with their own name.
