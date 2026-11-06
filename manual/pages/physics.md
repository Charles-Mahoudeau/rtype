# Physics

The physics plugin gives your game 2D bodies, colliders and collision events. Add it in `project.luau`:

```luau
engine.settings.plugins = { "physics", "physicsDebug" }
engine.settings.physics.gravity = { 0, 0 }     -- { x, y }, world units per second squared, Y down
```

The settings are in [Project settings](project-settings.md). The simulation runs on the server. Clients build the same
bodies so that questions (raycasts, `moveAndSlide`) answer against the same scene, but they never step it.

## Components

They are ordinary components you spawn with `ctx.commands` and query by name.

| Component | What it is |
|---|---|
| `rtype.Collider` | A shape (`shape`: `0` box, `1` circle, `2` capsule, with `size`, `radius`...), a `layer` and a `mask` |
| `rtype.RigidBody` | How it moves: its `type` (for example `1` for kinematic), `gravityScale`, `fixedRotation`, `continuousCollision` |
| `rtype.Velocity` | `linear` and `angular` velocity, in world units per second |
| `rtype.Transform` | The pose; write it to move a body |

`Collider` and `RigidBody` are replicated by the engine, so a client sees the server's bodies. A body's velocity and
position are set by writing its `rtype.Velocity` and `rtype.Transform`.

```luau
ctx.commands:spawn({
	["game.Laser"] = { position = muzzle, shooter = player },
	["rtype.Transform"] = { position = muzzle },
	["rtype.Collider"] = { size = { x = 60, y = 12 }, layer = 2, mask = 1 },
	["rtype.RigidBody"] = { gravityScale = 0, fixedRotation = true, continuousCollision = true },
	["rtype.Velocity"] = { linear = { x = 600, y = 0 } },
})
```

### Layers

A collider belongs to the categories in `layer` and collides with those in `mask`: 15 categories, bits 0 to 14. Two
colliders collide only when each one's layer is in the other's mask. Use `bit32.bor` to combine them.

## Events

Declare them under `events = { read = {...} }` and read them with `ctx.events.read`:

| Event | When |
|---|---|
| `rtype.CollisionStarted`, `rtype.CollisionEnded` | two bodies start or stop touching (`first`, `second` are entities) |
| `rtype.TriggerEntered`, `rtype.TriggerExited` | a body enters or leaves a sensor |

## Calls

List the ones you use under `calls` and make them with `ctx.call`.

| Call | Arguments | Result |
|---|---|---|
| `rtype.physics.raycast` | `origin`, `direction`, `maxDistance`, `mask`, `ignore`, `includeSensors` | `{ hit, entity, point, normal, distance }` |
| `rtype.physics.shapeCast` | the shape fields of a `Collider`, `from`, `direction`, `maxDistance`, the filter | as `raycast` |
| `rtype.physics.overlap` | the shape fields, `at`, the filter | `{ count, first }` |
| `rtype.physics.moveAndSlide` | `entity` (its `Collider` is the shape, its mask the filter), `from`, `delta`, `skin`, `maxSlides` | the position it ends at |
| `rtype.physics.applyForce`, `applyImpulse` | `entity`, `value` | nothing |
| `rtype.physics.applyTorque` | `entity`, `torque` | nothing |
| `rtype.physics.setGravity` | `gravity` | nothing |

A null `ignore` (the default) skips no entity.

## Ordering your systems

The physics runs in `FixedUpdate`. Order the systems that touch bodies around its steps:

- moving things by `rtype.Transform` or `rtype.Velocity`: `after = { "rtype.Physics.SyncBodies" }`,
  `before = { "rtype.Physics.SyncToPhysics" }`;
- reading positions the physics wrote: `after = { "rtype.Physics.SyncFromPhysics" }`;
- reacting to contacts: `after = { "rtype.Physics.Events" }`.

```luau
engine.ecs.system("game.Ships", {
	phase = "FixedUpdate",
	query = { write = { "rtype.Transform" }, with = { "game.Ship" } },
	calls = { "rtype.physics.moveAndSlide" },
	after = { "rtype.Physics.SyncBodies" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		for ship, transform in ctx.query do
			transform.position = ctx.call("rtype.physics.moveAndSlide", {
				entity = ship, from = transform.position, delta = { x = 4, y = 0 },
			})
		end
	end,
})
```

## The debug view

With `"physicsDebug"` in `plugins` (client only), F3 toggles outlines of the colliders and arrows for the velocities.
`physicsDebug.drawAll = true` outlines every collider; otherwise only entities with an `rtype.PhysicsDebug` component are
drawn.
