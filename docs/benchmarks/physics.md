# C/C++ physics engine benchmark (October 2026)

**Context:** our engine is C++23, built with `xmake`, around our own ECS (`rtype::ecs`) with GLM math. The game is
**2D for now** (`Transform` holds a `glm::vec2`), and **moving to 3D later is mandatory**, so the physics engine must
cover both without a rewrite. We need `Collider` and `RigidBody` components, triggers and raycasts.

What our architecture imposes on the choice (see [`network/design.md`](../network/design.md)):

- **Authoritative server.** The server simulates, clients display snapshots. Lockstep is rejected, so strict
  cross-machine determinism is a bonus, not a requirement.
- **One thread per room.** Each room is its own `App` running a fixed 60 Hz loop, and a server process hosts
  several rooms. The physics world lives inside a room and must not spawn its own threads per room.
- **Client prediction.** The client replays a few predicted systems on its own ship only, `N` times in one frame.
  It cannot step a whole physics world for that.
- **Headless server.** The `server` executable never links the window or the renderer.
- **Three platforms, three compilers.** Linux, macOS and Windows; Clang, Apple Clang and MSVC must build it.

---

## 1. Overview

| Library | Dim. | Language / API | License | Status (Oct. 2026) | Maturity | `xmake-repo` package |
|---|---|---|---|---|---|---|
| **Jolt Physics** | 3D (+ 2D via `Plane2D`) | C++17 | MIT | Very active, v5.6 (July 2026) | ★★★★★ | `joltphysics` |
| **Box2D v3** | 2D | C (handles) | MIT | Active, v3.1.x (3.2 in progress) | ★★★★★ | `box2d` |
| **Box3D** | 3D | C (same design as Box2D v3) | MIT | **Alpha**, released late June 2026 | ★★☆☆☆ | none |
| **PhysX 5** | 3D | C++ | BSD-3 | Maintained by NVIDIA | ★★★★★ | none |
| **Bullet 3** | 3D | C++ (old style) | zlib | Mostly in maintenance | ★★★★☆ | `bullet3` |
| **ReactPhysics3D** | 3D | C++ | zlib | Few releases (0.10.x) | ★★★☆☆ | none |
| **Chipmunk2D** | 2D | C | MIT | Nearly abandoned | ★★★☆☆ | `chipmunk2d` |
| **Newton Dynamics 4** | 3D | C++ | zlib | Active but niche | ★★★☆☆ | none |
| **Rapier** | 2D + 3D | Rust (no native C++) | Apache-2 | Very active | ★★★★☆ | none |
| **ODE / qu3e** | 3D | C / C++ | BSD / zlib | Historical / educational | ★★☆☆☆ | `ode` |

The package column comes from our local `xmake-repo` index (May 2026); "none" means a custom package rule in our
`xmake.lua`.

---

## 2. Comparison on our criteria

Scores out of 5, applied to **our** project. 3D being mandatory, "3D later" and "one library for 2D and 3D" weigh
as much as "2D today".

| Criterion | Jolt | Box2D v3 → Box3D | PhysX 5 | Bullet 3 | ReactPhysics3D |
|---|---|---|---|---|---|
| 2D quality today | 3 (simulated 2D) | **5** (true 2D) | 2 | 2 | 2 |
| 3D quality later (mandatory) | **5** | 2 (alpha) | 5 | 3 | 3 |
| One library for 2D and 3D | **5** | 3 (two libraries, a migration) | 3 (axis locks) | 2 | 2 |
| ECS integration | 4 (`BodyID` + user data) | **5** (handles + polled events) | 3 | 2 (pointers, inheritance) | 3 |
| Fits one thread per room | **5** (single-threaded job system) | **5** (worker count of 1) | 3 | 3 | 4 |
| Fits our netcode (server + prediction) | **5** (`CharacterVirtual`, shape casts) | 4 | 4 | 2 | 3 |
| `xmake` integration | **5** (package with options) | 3 (Box3D has no package) | 1 (heavy, no package) | 4 | 3 |
| Documentation / samples | 4 (very complete samples) | 5 (Box2D) / 2 (Box3D) | 4 | 3 | 4 |
| Longevity | **5** | 5 / 3 | 4 | 2 | 2 |
| **Total /45** | **41** | **32** (with Box3D's scores) | 29 | 23 | 26 |

---

## 3. Library details

### Jolt Physics

- **Pros:** a modern, very fast engine used in AAA games, and Godot's default 3D engine since 4.6. It has every
  shape we will need (box, sphere, capsule, convex hull, mesh, heightfield, compound), sensors (triggers), a
  character controller (`CharacterVirtual`), raycasts and shape casts, and an optional cross-platform deterministic
  mode.
- **2D:** `EAllowedDOFs::Plane2D` locks Z and the X/Y rotations. It works well for classic 2D gameplay such as a
  shoot'em up, but it is not true 2D: no native 2D polygons or chains. R-Type needs boxes, circles and capsules,
  which `Plane2D` covers.
- **Threading:** Jolt runs its work through a `JobSystem` we choose. With `JobSystemSingleThreaded` (or a small pool
  shared by all rooms), a room steps its world on its own thread and the `ContactListener` is called on that
  thread too, so a plain `std::vector` buffer of events is enough, with no mutex.
- **Cons:** Jolt has its own math types (`JPH::Vec3`, `JPH::Quat`), so we need a GLM conversion header. It is Y-up,
  while our 2D `Transform` is Y-down: the conversion header flips the axis. Jolt needs `RegisterTypes()` and a
  `Factory` once per process before any room starts, outside the ECS (which keeps no global state).

### Box2D v3, then Box3D later

- **Pros:** Box2D v3 is the 2D reference: more than twice as fast as v2.4, deterministic and multithreaded. Its
  **handle** API (`b2BodyId`) with **events polled after the step** suits an ECS: no callbacks, just buffers to read.
- Box3D keeps the **same architecture and API** (`b3*`) as Box2D v3, so the 2D → 3D move would mostly be a rename.
  It is already used in production (Facepunch's s&box).
- **Cons for us:** since 3D is mandatory, this path means **two integrations and a migration** on the schedule, and
  the target library is **in alpha**: its author still plans API changes, and its documentation and character
  controller are incomplete. Box3D has no `xmake-repo` package, and Box2D 3.2 brings breaking changes. Its main
  strength, strict determinism, matters little with an authoritative server.

### PhysX 5

- **Pros:** extremely complete (articulations, vehicles, GPU) and open source under BSD-3.
- **Cons:** heavy build, no `xmake` package, verbose API, 3D only. Overkill for a student project.

### Bullet 3

- **Pros:** well known and widely documented on the web, and packaged for `xmake`.
- **Cons:** dated API (pointers, inheritance, `btVector3`), slower than Jolt, and development has slowed down.
  Nothing justifies picking it in 2026.

### ReactPhysics3D

- **Pros:** simple, readable, no dependencies, good for learning.
- **Cons:** slower, fewer features and few releases. 3D only.

### Chipmunk2D, Newton, Rapier, ODE, qu3e

- **Chipmunk2D:** 2D only, superseded by Box2D v3.
- **Newton:** community too small.
- **Rapier:** excellent, but written in Rust. The C/C++ bindings are limited and would add a Rust toolchain to a
  build that must run on three platforms.
- **ODE / qu3e:** historical or educational, not for a real project.

---

## 4. Fitting it into our engine

### Components and systems

The components stay **descriptors** (`Collider`, `RigidBody`, declared with `RTYPE_ECS_COMPONENT` like `Transform`),
and an internal `PhysicsHandle` component links the entity to its body. The body stores the entity in its user data
(`Body::SetUserData`) to go back from a contact to the entity.

The physics world is a resource of the room's `App`. The systems run in `FixedUpdate`, at the room's 60 Hz step, in
this order:

1. create the bodies of new `RigidBody` / `Collider` entities;
2. sync ECS → physics (kinematic bodies, teleports);
3. step the world once (one fixed step);
4. sync physics → ECS: write the body positions to `Transform`;
5. turn the buffered contacts and sensor overlaps into ECS events;
6. destroy the bodies of removed entities.

`TransformPlugin` then computes `GlobalTransform` in `PostUpdate`, as it already does.

| | Jolt | Box2D / Box3D |
|---|---|---|
| Handle stored in the component | `JPH::BodyID` | `b2BodyId` / `b3BodyId` |
| Finding the entity from the body | `Body::SetUserData(entity)` | `b2Body_SetUserData(id, ptr)` |
| Collisions / triggers | `ContactListener`, on the room thread with a single-threaded job system | `b2World_GetContactEvents()` / `GetSensorEvents()` after the step |
| Transform sync | `BodyInterface::GetPositionAndRotation` on active bodies | `b2World_GetBodyEvents()` (only bodies that moved) |

### Server, client and prediction

- **The server owns the simulation.** Only the room's world steps physics; positions reach the clients through the
  replicated `Transform`. A client needs no dynamic bodies of the other entities.
- **Prediction does not step the world.** A predicted system replays `N` inputs on one ship in one frame. It moves
  the ship with a `CharacterVirtual` or a shape cast against the colliders at their interpolated positions, which
  costs a few queries instead of `N` world steps. The server runs the same code, so both agree in the common case.
- **Determinism:** Jolt's `cross_platform_deterministic` option is not required (the server corrects), but turning
  it on keeps prediction corrections rare across platforms. To measure before deciding.

### Build

- `add_requires("joltphysics")` in the target that integrates it, with `debug_renderer` turned off for the server.
  Jolt is built without exceptions and RTTI by default, which suits a dependency we only call into.
- **Open question: where the integration lives.** Our dependency rules say `plugins` depends on nothing but `ecs`
  and glm, so a `plugins/physics/` folder linking Jolt would break them. Two options: a new `physics` target
  (`ecs` ← `physics` ← `engine`), or an explicit exception for Jolt in `plugins`. To decide as a team before
  writing the code.

---

## 5. Conclusion

### Recommendation: **Jolt Physics**

It is the best fit for **our** project:

1. **One library, one ECS integration.** We write the physics plugin once. Moving to 3D means dropping `Plane2D`,
   adding shapes and widening `Transform` to 3D: no rewrite of the integration.
2. **Mature in 3D today**, while 3D is mandatory for us and Box3D is still in alpha.
3. **Fits our architecture:** a single-threaded job system per room, contact callbacks on the room thread,
   `CharacterVirtual` for prediction, and a ready `xmake` package.
4. **Enough for an R-Type in 2D** (boxes, spheres, capsules, triggers, raycasts).

**Two points to watch:** the GLM ↔ Jolt conversion header (with the Y-axis flip), and where the integration lives
in our dependency graph (see [Build](#build)).

### Box2D v3 → Box3D: not recommended

Box2D v3 is the best 2D engine, and its API is the most pleasant with an ECS. But for us, 3D is not optional: this
path means integrating Box2D, then migrating to Box3D, a library in alpha with no `xmake` package and an API that
will still change. Its strongest argument, strict determinism, matters little with an authoritative server. We keep
an eye on Box3D, but we do not build on it.

### To avoid

Bullet (dated), PhysX (overkill, heavy build), Chipmunk (2D only, abandoned), Rapier (Rust).
