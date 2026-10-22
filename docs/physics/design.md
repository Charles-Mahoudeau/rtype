# Physics Design

This document describes the physics of the R-Type engine as implemented: a 2D physics plugin for the ECS, built on
[Jolt Physics](https://github.com/jrouwe/JoltPhysics). It covers the components, the systems and their order, the
events, the scene queries, the network modes and how gameplay uses them. Why Jolt, and the alternatives that were
weighed, are in [`../benchmarks/physics.md`](../benchmarks/physics.md); this document records what was built from
that plan and where it differs.

> Snippets omit the Epitech header and most `[[nodiscard]]`/`noexcept` for brevity. Names are relative to
> `rtype::plugins::physics` unless qualified.

## Contents

1. [Scope](#1-scope)
2. [Where it lives](#2-where-it-lives)
3. [Units and axes](#3-units-and-axes)
4. [Components](#4-components)
5. [The plugin and its systems](#5-the-plugin-and-its-systems)
6. [Bodies and syncing](#6-bodies-and-syncing)
7. [Collision layers](#7-collision-layers)
8. [Contact events](#8-contact-events)
9. [Scene queries and `moveAndSlide`](#9-scene-queries-and-moveandslide)
10. [Network modes and prediction](#10-network-modes-and-prediction)
11. [Debug view](#11-debug-view)
12. [Performance](#12-performance)
13. [Using it from gameplay](#13-using-it-from-gameplay)
14. [Differences with the benchmark's plan](#14-differences-with-the-benchmarks-plan)
15. [Toward 3D](#15-toward-3d)

## 1. Scope

- **2D first.** Bodies move in the XY plane (Jolt's `EAllowedDOFs::Plane2D`); every shape is a 2D shape extruded
  in Jolt. 3D is the next milestone (see [Toward 3D](#15-toward-3d)).
- **An ECS plugin, nothing else.** Gameplay never sets Jolt up, never sees a Jolt type and never steps a world: it
  writes components, reads events and calls a resource.
- **The server is authoritative.** Only the server (and a standalone game) simulates. Clients keep query-only bodies
  for prediction and the debug view.

## 2. Where it lives

```
src/rtype/plugins/physics/          namespace rtype::plugins::physics
├── PhysicsPlugin.hpp/.cpp          the plugin: registers, inserts the world, adds the systems
├── PhysicsWorld.hpp/.cpp           the resource: one Jolt world (pImpl, no Jolt in the header)
├── PhysicsSettings.hpp             how the world is built
├── Collider.hpp, RigidBody.hpp     what gameplay writes
├── Velocity.hpp                    opt-in speed, read and written back
├── PhysicsHandle.hpp               internal: entity → body
├── CollisionEvents.hpp             CollisionStarted/Ended, TriggerEntered/Exited
├── QueryFilter.hpp                 what a scene query sees
├── MoveAndSlide.hpp/.cpp           kinematic move for predicted systems
├── ComponentOptions.hpp            how Collider and RigidBody are registered (replicated)
├── exceptions/PhysicsExceptions.hpp
├── detail/                         Jolt only: runtime, math, layers, body settings, contacts, query filters
└── debug/                          namespace physics::debug: PhysicsDebug, PhysicsDebugSettings, PhysicsDebugPlugin
```

- **Jolt is a private package of the `plugins` target** (`joltphysics 5.6.x`, `object_layer_bits = "32"`, declared
  in `src/rtype/plugins/xmake.lua`). Only `.cpp` files and `detail/` include it; no public header does, so
  `plugins` still exposes nothing but the ECS and glm to its users. The benchmark's open question (a separate
  `physics` target or an exception in `plugins`) was settled this way: Jolt is linked once, in the static
  `plugins` library, which matters because Jolt keeps process-wide globals (allocator, factory, registered types).
- **`detail::JoltRuntime`** sets those globals up once per process and tears them down when the last world goes. It
  is refcounted and guarded by a mutex, so several rooms can build their worlds on their own threads.
- **No dependency on the network.** `plugins` includes nothing from `network` or `replication`; replication is an ECS
  option on two components (see [Network modes](#10-network-modes-and-prediction)).

## 3. Units and axes

The API speaks the engine's units; the conversion happens inside `PhysicsWorld` (`detail/JoltMath.hpp`).

| | Engine (API) | Jolt |
|---|---|---|
| Distance | world units (pixels today) | meters: `world / PhysicsSettings::unitsPerMeter` (32 by default) |
| Y axis | down, like `Transform` | up |
| Angles | radians, clockwise on screen | radians, counter-clockwise around Z |
| Speed | world units per second | meters per second |

Jolt's margins and sleep thresholds are tuned for objects about a meter big, hence `unitsPerMeter`: a 32-pixel ship
is one meter.

## 4. Components

All are plain data, reflected with `RTYPE_ECS_COMPONENT`, so they can be replicated and written from Luau.

| Component | Written by | Replicated | Role |
|---|---|---|---|
| `Collider` | gameplay | every change | shape (`kBox` size, `kCircle` radius, `kCapsule` radius + halfHeight), offset, sensor, friction, restitution, `layer` / `mask` |
| `RigidBody` | gameplay | every change | `kStatic` / `kKinematic` / `kDynamic`, mass, damping, gravity scale, `fixedRotation`, `continuousCollision` |
| `Velocity` | gameplay and the plugin | no | linear (units/s) and angular (rad/s) speed; opt-in |
| `PhysicsHandle` | the plugin | no | the Jolt body id, meaningful in its own world only |

- **An entity gets a body when it has a `Collider` and a `Transform`.** Without a `RigidBody` it is static.
- **`Velocity` is opt-in.** Only entities that have one are synced: a `Velocity` gameplay changed is given to the body
  before the step, and a dynamic body writes its velocity back after it.
- **Forces and impulses are calls, not components:** `PhysicsWorld::applyForce`, `applyImpulse` and `applyTorque`
  queue them for the next step, which then forgets them.

## 5. The plugin and its systems

```c++
app.addPlugins(rtype::ecs::MinimalPlugins{})  // Time: the fixed tick the physics steps with
    .addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = {0.0F, 0.0F}}});
```

`PhysicsPlugin` adds the `TransformPlugin`, registers the components and the four events, inserts the
`PhysicsWorld` resource, and adds five systems to `FixedUpdate`, in this order:

| System | Side | Does |
|---|---|---|
| `rtype.Physics.SyncBodies` | every side | one body per entity with a `Collider` and a `Transform`: creates, rebuilds (when `Collider` or `RigidBody` changed), destroys (despawned, component removed); keeps `PhysicsHandle` |
| `rtype.Physics.SyncToPhysics` | every side | server: kinematic bodies move to their `Transform`, others follow a `Transform` gameplay moved (teleport), changed `Velocity`s reach the bodies. Client mode: every body is teleported to its `Transform` |
| `rtype.Physics.Step` | server | applies the queued forces and impulses, steps the world by `Time::fixedDelta` |
| `rtype.Physics.SyncFromPhysics` | server | writes the pose of the bodies that moved to `Transform`, the velocity of dynamic bodies to `Velocity` |
| `rtype.Physics.Events` | server | sends the contacts of the step as ECS events |

"Server" means `Side::kServer`, so standalone runs it too. Game systems place themselves by these names, which are
public constants of `PhysicsPlugin`:

- **moving things by `Transform` or `Velocity`:** after `kSyncBodiesSystem`, before `kSyncToPhysicsSystem`;
- **forces and impulses:** before `kStepSystem`;
- **reading positions the physics wrote:** after `kSyncFromPhysicsSystem`;
- **reacting to contacts:** after `kEventsSystem`.

`TransformPlugin` then computes `GlobalTransform` in `PostUpdate`, as for any entity.

## 6. Bodies and syncing

- **Rebuild on change.** A changed `Collider` or `RigidBody` rebuilds the body (Jolt shapes are immutable); a moving
  body keeps its velocity. Its contacts restart, so a rebuilt pair sends no `CollisionEnded`.
- **Destroy without callbacks.** The ECS does not report despawns or removed components, so `SyncBodies` runs a body
  pass: it touches every entity that still qualifies, and `endBodyPass()` destroys the bodies it did not see.
- **Echo-free sync.** The world remembers the last pose and velocity it gave each body or read from it
  (`isOutOfSync`, `isVelocityOutOfSync`). A `Transform` the physics wrote itself is therefore never taken for a
  teleport by gameplay, and only bodies whose pose changed write their `Transform`: sleeping bodies never mark it
  changed, so replication does not send them.
- **Kinematic bodies** move to their `Transform` with `MoveKinematic` (they push dynamic bodies on their way), or at
  their `Velocity` on the ticks gameplay did not move them.

## 7. Collision layers

`Collider::layer` names the categories a collider belongs to and `Collider::mask` the ones it collides with: 15
categories, bits 0 to 14 (`kAllLayers`). **Two colliders collide only when each one's layer is in the other's mask**,
and two static bodies never do. Each game names its own categories:

```c++
constexpr std::uint32_t kObstacle = 1U << 0;
constexpr std::uint32_t kLaser = 1U << 1;
Collider{.shape = ShapeType::kBox, .size = {60.0F, 12.0F}, .layer = kLaser, .mask = kObstacle | kEnemyShip};
```

Inside Jolt, layer, mask and a static flag are packed in a 32-bit `ObjectLayer` (`detail/Layers`: bits 0–14
categories, 15–29 mask, 30 static), which is why the package is built with `object_layer_bits = "32"`. The static flag
picks the broad-phase tree, so static bodies never cost a pair test against each other.

## 8. Contact events

| Event | Sent when | Fields |
|---|---|---|
| `CollisionStarted` | two solid colliders start touching | `first`, `second`, `point`, `normal` (from first to second) |
| `CollisionEnded` | they stop touching | `first`, `second` |
| `TriggerEntered` | a collider enters a sensor | `trigger` (the sensor), `other` |
| `TriggerExited` | it leaves | `trigger`, `other` |

- **One start and one end per pair of bodies**, whatever the number of contact points or sub-shapes
  (`detail::ContactCollector` counts them).
- **Collected during the step, sent after it.** Jolt calls its `ContactListener` inside the step; the collector buffers
  the pairs and `rtype.Physics.Events` sends them on the room's thread, in the ECS.
- **Sleep is not a separation.** Jolt removes the contacts of bodies that fall asleep; the collector waits until after
  the step (`settle()`) to tell a real separation from a pair that merely went to sleep, so resting bodies do not send
  false `CollisionEnded`.
- **No end for destroyed bodies.** A despawned entity, a removed collider or a rebuilt body forgets its pairs.

## 9. Scene queries and `moveAndSlide`

`PhysicsWorld` answers queries in world units, against the bodies as of the last step plus those created since:

| Call | Returns |
|---|---|
| `raycast(origin, direction, maxDistance, filter)` | the closest `RayHit` (entity, point, normal, distance) |
| `raycastAll(...)` | every body hit, closest first, one hit per body |
| `shapeCast(shape, from, direction, maxDistance, filter)` | the first body a swept `Collider` shape would touch |
| `overlap(shape, at, filter)` | the entities overlapping a shape placed somewhere |

`QueryFilter{.mask, .ignore, .includeSensors}` picks what a query sees: the categories it sees (one way, whatever the
bodies' own masks), an entity to skip (the shooter), and whether sensors count (no by default).

`moveAndSlide` moves a shape by a delta **without stepping the world**: each shape cast stops it a skin away from what
it hits (`SlideSettings::skin`, 0.5 units, kept along the surface's normal) and the rest of the delta slides along the
surface, up to `maxSlides` surfaces. A shape that starts inside a body stays put. The entity overload sees the
collider's mask and never collides with the entity's own body:

```c++
transform->position = moveAndSlide(*world, ship, collider, transform->position, delta);
```

Queries and `moveAndSlide` sweep shapes without rotation.

## 10. Network modes and prediction

```
                 server (RunMode::kServer)                       client (RunMode::kClient)
gameplay ─▶ Transform ─▶ SyncToPhysics ─▶ Step ─▶ Transform      replication ─▶ Transform
                         (bodies follow)  (Jolt)  (written back)                    │
                                                                  SyncToPhysics: teleport every body
replicated: Collider, RigidBody (+ the game's own components)     queries, moveAndSlide, debug view
```

- **What is replicated:** `Collider` and `RigidBody`, on every change, through `kColliderOptions` and
  `kRigidBodyOptions` (`ComponentOptions.hpp`). Every plugin that registers them uses these constants, because a
  component registered twice with other options is refused. `Velocity` and `PhysicsHandle` stay local.
- **The physics knows nothing of the network.** Replication is an `ecs::ComponentOptions` flag, which `replication`
  reads generically, as for any game component.
- **Clients build bodies but never step.** `SyncBodies` runs on every side; in client mode the plugin registers the
  teleporting variant of `SyncToPhysics` under the same name, and `Step`, `SyncFromPhysics` and `Events` never run.
  The client's bodies follow the `Transform` replication (or the game) writes, so scene queries answer against the
  same scene as the server. The same name matters: the schedule refuses unordered systems touching the same data even
  in a mode where one of them never runs, so a new system would break every game system placed around
  `SyncToPhysics`.
- **Prediction does not step.** A predicted system replays its inputs with `moveAndSlide`; the server runs the same
  code, so both agree in the common case (tested: a ship sliding along a wall ends at the same position on both
  sides). Collisions with other entities use their interpolated positions; a rare mismatch is corrected by the
  server.
- **Standalone** runs everything as the server does.

The duel game (`src/rtype/engine/main.cpp`) shows the whole loop: the server moves the ships with `moveAndSlide`
along the obstacles, the lasers are dynamic bodies whose `CollisionStarted` ends the game, and the clients add the
physics plugin too (so both sides register the same replicated components) plus the debug view on F3.

## 11. Debug view

`physics::debug::PhysicsDebugPlugin`, client side, after the `BackendPlugin`, draws collider outlines and velocity
arrows as `Shape`s (one `kRect` / `kCircle` / `kCapsule` per outline, three `kLine` per arrow), after the sprites:

- **per entity**, with a `PhysicsDebug{shape, velocity, color}` component, like gizmos in Unity;
- **for every collider**, with `PhysicsDebugSettings::drawAll`;
- **F3** (`toggleKey`) flips `enabled`, ignoring OS auto-repeat.

It draws from the components (`Collider`, `GlobalTransform`, `Velocity`), not from Jolt, so it shows where sprites are
drawn and works without bodies. It is a separate plugin and component on purpose: the headless server has no
renderer, and toggling debug must never rebuild a body.

## 12. Performance

`examples/plugins/PhysicsBenchmark` measures a room as the server runs it: an `App` in server mode with the physics
plugin, ticked by hand, one thread per room. The arena holds bullets (dynamic circles bouncing off walls and enemies,
never off each other) and enemies (kinematic boxes patrolling), one enemy for four bullets.

```sh
xmake f -m release --PhysicsBenchmark=y && xmake run PhysicsBenchmark   # --quick, --runs N, --steps N
```

Results on an Apple M2 (4 performance + 4 efficiency cores), release build, median of 5 runs of 300 ticks; "tick" is
the whole plugin (sync + step + write-back + events), "step" Jolt's part of it:

| Bodies per room | Rooms | Tick (ms) | Step (ms) | µs per body | p99 tick (ms) | Rooms per core at 60 Hz |
|---:|---:|---:|---:|---:|---:|---:|
| 100 | 1 | 0.043 | 0.022 | 0.43 | 0.07 | ~390 |
| 100 | 8 | 0.049 | 0.024 | 0.49 | 0.19 | ~340 |
| 1 000 | 1 | 0.57 | 0.36 | 0.57 | 0.79 | ~29 |
| 1 000 | 4 | 0.60 | 0.37 | 0.60 | 0.74 | ~28 |
| 1 000 | 8 | 0.64 | 0.39 | 0.64 | 1.7 | ~26 |
| 5 000 | 1 | 5.9 | 4.7 | 1.2 | 7.6 | ~3 |
| 5 000 | 4 | 6.1 | 4.9 | 1.2 | 7.7 | ~3 |
| 5 000 | 8 | 8.5 | 6.4 | 1.7 | 18–20 | ~2 |

- **A room of a few hundred bodies costs well under a millisecond per tick**: the R-Type room budget is not the
  physics. 1 000 bodies use under 4 % of a core at 60 Hz.
- **Rooms scale with cores**, one thread each, with no shared state but the refcounted runtime. On the M2, 8 rooms
  land on the efficiency cores too, hence the slower 8-room rows.
- **The ECS share** (tick minus step: syncing `Transform`, `Velocity`, events) is about a third of the tick at 1 000
  bodies and a fifth at 5 000.
- **Reproducibility:** ticks are summed up by their median, which a preempted tick does not move; two full runs on the
  same machine agree within about 5 %. Capacities (`maxBodyPairs`, `maxContactConstraints`) are sized per room by the
  benchmark; a row would say "contacts dropped" if Jolt ran out.

## 13. Using it from gameplay

```c++
// A wall, a dynamic crate and a kinematic ship.
commands.spawn().add(Transform{.position = {400.0F, 580.0F}}).add(Collider{.size = {800.0F, 40.0F}});
commands.spawn()
    .add(Transform{.position = {200.0F, 100.0F}})
    .add(Collider{.shape = ShapeType::kBox, .size = {48.0F, 48.0F}})
    .add(RigidBody{.mass = 10.0F});
commands.spawn()
    .add(Transform{.position = {80.0F, 300.0F}})
    .add(Collider{.shape = ShapeType::kCapsule, .radius = 8.0F, .halfHeight = 16.0F, .layer = kShip, .mask = kScenery})
    .add(RigidBody{.type = BodyType::kKinematic})
    .add(Velocity{.linear = {300.0F, 0.0F}});

// A kick, before the step.
void kick(rtype::ecs::ResMut<PhysicsWorld> world, ...) { world->applyImpulse(crate, {2000.0F, -4000.0F}); }

// Hits, after the events.
void onHit(rtype::ecs::EventReader<CollisionStarted> hits) { for (const CollisionStarted& hit : hits) { ... } }

// A laser: the first enemy or wall ahead of the ship, never the ship itself.
const auto hit = world->raycast(muzzle, {1.0F, 0.0F}, 1000.0F, QueryFilter{.mask = kEnemy | kWall, .ignore = ship});
```

Errors (invalid shapes, layers or settings, a full world) throw `exceptions::PhysicsException`.

Examples: `examples/plugins/physics` (headless scene: every body type, layers, events, a raycast, `moveAndSlide`),
`examples/plugins/PhysicsDebug` (the debug view in a window), `examples/plugins/PhysicsBenchmark`, and the duel game in
`src/rtype/engine/`. Tests: `tests/plugins/Physics*Tests.cpp` and `Jolt*Tests.cpp`.

## 14. Differences with the benchmark's plan

| Plan ([`../benchmarks/physics.md`](../benchmarks/physics.md#4-fitting-it-into-our-engine)) | Built |
|---|---|
| Body destruction as a sixth system after the events | Part of `SyncBodies` (body pass), before the step: a despawned entity never steps again |
| Positions reach clients through a replicated `Transform`; clients have no bodies | `Transform` is not replicated yet (games replicate their own position fields); clients keep query-only bodies that follow `Transform` |
| Prediction with `CharacterVirtual` or a shape cast | `moveAndSlide`, on `shapeCast` |
| `debug_renderer` turned off for the server | Kept at its default: the option does not reach Jolt's CMake, and a mismatch aborts on Jolt's version check |
| Open question: a `physics` target or an exception in `plugins` | Jolt is a private package of `plugins`; no public header includes it |
| `cross_platform_deterministic` to measure | Not enabled: the server corrects |

## 15. Toward 3D

The 3D milestone keeps the API's shape: `glm::vec3` positions and rotations in `Transform`, 3D shapes in `Collider`,
`EAllowedDOFs::All`, and the same systems, events and queries. What stays as is: the plugin and its system names, the
body pass, layers, the contact collector, the network modes and `moveAndSlide`'s algorithm. What changes: the
conversions in `detail/JoltMath.hpp`, the shape builder in `detail/BodySettings`, and the debug view, which needs 3D
lines.
