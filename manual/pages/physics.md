# Physics

The physics plugin gives your game 2D bodies, colliders and collision events. Add it in `project.luau`:

```luau
engine.settings.plugins = { "rtype.Physics", "rtype.PhysicsDebug" }
engine.settings.physics.gravity = { 0, 0 }     -- { x, y }, world units per second squared, Y down
```

The settings are in [Project settings](project-settings.md). The simulation runs on the server. Clients build the same
bodies so that questions (raycasts, `moveAndSlide`) answer against the same scene, but they never step it.

## Components

They are ordinary components you spawn with `ctx.commands` and query by name.

| Component | What it is |
|---|---|
| `rtype.Collider` | A shape, its material and its collision categories (fields below) |
| `rtype.RigidBody` | How it moves: static, kinematic or dynamic, its mass and damping (fields below) |
| `rtype.Velocity` | `linear` and `angular` velocity, in world units per second |
| `rtype.Transform` | The pose; write it to move a body |

An entity with a `Collider` and a `Transform` gets a body; without a `RigidBody`, it is a static body (a wall).

`rtype.Collider`, the shape and the material. Each shape reads its own fields: `size` for a box, `radius` for a circle,
`radius` and `halfHeight` for a capsule.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `shape` | u8 | `0` | `0` box, `1` circle, `2` capsule |
| `size` | vec2 | `{ x = 32, y = 32 }` | box: full width and height |
| `radius` | f32 | `16` | circle and capsule: the radius |
| `halfHeight` | f32 | `16` | capsule: half the length of its straight part, along Y |
| `offset` | vec2 | `{ x = 0, y = 0 }` | the shape's center relative to the `Transform` |
| `sensor` | bool | `false` | detects overlaps (trigger events) without colliding |
| `friction` | f32 | `0.2` | how much the surface resists sliding, usually 0 to 1 |
| `restitution` | f32 | `0` | how bouncy it is: 0 absorbs, 1 bounces back fully |
| `layer` | u32 | `1` | the [collision categories](#layers) it belongs to |
| `mask` | u32 | every category (`0x7FFF`) | the categories it collides with |

`rtype.RigidBody`, how the body moves:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `type` | u8 | `2` | `0` static (never moves), `1` kinematic (moved by your script through its `Transform`, pushes dynamic bodies), `2` dynamic (gravity, forces, collisions) |
| `mass` | f32 | `1` | in kilograms; only a dynamic body uses it |
| `linearDamping` | f32 | `0.05` | slows the velocity down over time, per second |
| `angularDamping` | f32 | `0.05` | slows the rotation down over time, per second |
| `gravityScale` | f32 | `1` | multiplies the world's gravity; `0` ignores it |
| `fixedRotation` | bool | `false` | never rotates |
| `continuousCollision` | bool | `false` | never tunnels through thin colliders when fast (bullets), at a cost |

Distances are in world units, Y down. `Collider` and `RigidBody` are replicated by the engine, so a client sees the server's bodies. A body's velocity and
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

With `"rtype.PhysicsDebug"` in `plugins` (client only), F3 toggles outlines of the colliders and arrows for the velocities.
`physicsDebug.drawAll = true` outlines every collider; otherwise only entities with an `rtype.PhysicsDebug` component are
drawn.
