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

- Each `[node]` is one entity. `path="."` is the root and comes first; `"LeftTurret"` is a child of the root,
  `"LeftTurret/Muzzle"` a child of the turret, and a parent comes before its children.
- `instance="actors/turret.actor"` spawns that capsule there. Write only the fields you change: the others keep the
  turret's values.
- `side=client` (or `both`) for what only players see; nodes are the server's by default.
- A component is `name = { field = value, ... }`, with the component's defaults for the fields you leave out, and `{}`
  for all of them. Values: numbers, `true` / `false`, `"text"` (also asset paths), `(x, y)` vectors and `[a, b]`
  lists. `#` starts a comment.
- A file the engine cannot read stops the start with the file, the line and the column of the mistake.

### Hot reload

While the game runs in a debug build, save a capsule file and the engine reads it again, with the files that use it,
and respawns what came from it in place. A file saved with a mistake keeps its previous version and prints why. Turn it
off, or on in a release build, with `engine.settings.capsule = { hotReload = false }`.

## Spawning, switching levels, reloading

Three engine calls, listed in a system's `calls`:

| Call | Arguments | Result | What it does |
|---|---|---|---|
| `rtype.capsule.spawn` | `capsule` (a path) | the root entity | Spawns a copy of the capsule |
| `rtype.capsule.load` | `capsule` (a path) | the root entity | Switches levels: the capsule loaded before is despawned, then this one is spawned |
| `rtype.capsule.reload` | `root` (an entity) | `true` if it was a capsule's root | Spawns the capsule's current version again into the same root |

```luau
engine.ecs.system("game.Waves", {
	phase = "Update",
	query = {},
	calls = { "rtype.capsule.spawn", "rtype.capsule.load" },
	run = function(ctx)
		local bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
		ctx.commands:add(bydo, "rtype.Transform", { position = { x = 820, y = 140 } })

		if bossDefeated then
			ctx.call("rtype.capsule.load", { capsule = "scenes/level2.scene" })
		end
	end,
})
```

- The root is ready to use at once: add components to it with `ctx.commands`, they replace the capsule's (the whole
  component: its defaults with the fields you give).
- The entities appear when the system's commands are applied, at the end of its phase, like `ctx.commands:spawn`.
- Despawning the root (`ctx.commands:despawn(root)`) removes the whole capsule.
- `load` only removes the capsule loaded before: your player and anything else spawned outside it stay.
- A reload despawns the root's children, including ones you attached to it, and keeps the root's other components.

## Sides

Each node is spawned on its side only, so one capsule loads on the server and on every client without duplicates: the
server's entities reach the players by replication, and the client nodes stay local. Components marked server-only
are left out on clients, and client-only ones on a dedicated server.

## Errors

A call on a path no capsule has, or a capsule that contains itself through its instances, is a Luau error naming the
call. Nothing is spawned then.
