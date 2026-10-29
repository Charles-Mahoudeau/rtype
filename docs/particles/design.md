# Particles Design

This document describes the 2D particle system of the R-Type engine as planned: a thin ECS plugin, `ParticlePlugin`,
that drives emitters from the world, and a renderer that simulates and draws their particles on the GPU through a
contract of `IRenderer`. Each effect chooses where it runs, CPU or GPU, and an effect that asks for the GPU falls back to
the CPU when the renderer cannot simulate it, so every backend can show every effect.

> Status: **design agreed, nothing implemented yet.** Snippets omit the Epitech header and most `[[nodiscard]]`/
> `noexcept` for brevity. Names are relative to `rtype::plugins::particle` unless qualified; `graphics::` is
> `rtype::plugins::backend::graphics`. Decisions and what is still open are in [Open questions](#14-open-questions).

## Contents

1. [Scope](#1-scope)
2. [Why the GPU, and why a CPU path too](#2-why-the-gpu-and-why-a-cpu-path-too)
3. [Where it lives](#3-where-it-lives)
4. [Effects: `ParticleEffect`](#4-effects-particleeffect)
5. [The renderer contract](#5-the-renderer-contract)
6. [Choosing CPU or GPU](#6-choosing-cpu-or-gpu)
7. [The plugin: resources, components, events, systems](#7-the-plugin-resources-components-events-systems)
8. [The CPU simulation](#8-the-cpu-simulation)
9. [The GPU simulation (Vulkan)](#9-the-gpu-simulation-vulkan)
10. [Keeping both paths alike](#10-keeping-both-paths-alike)
11. [Sub-emitters](#11-sub-emitters)
12. [Luau](#12-luau)
13. [Implementation plan](#13-implementation-plan)
14. [Open questions](#14-open-questions)

## 1. Scope

- **A general-purpose engine, not a shoot'em up only.** Tens of thousands of particles on screen (smoke, sparks,
  debris, rain, magic) without the CPU noticing, not a few hundred.
- **Effects are data.** An effect is a value describing how particles spawn, move, age and look; it is built in C++
  first, loaded from Luau files later ([Luau](#12-luau)).
- **Gameplay never sees where particles live.** It adds a `ParticleEmitter` to an entity and plays or stops it; whether
  the particles are simulated by the CPU or by a compute shader changes nothing for it.
- **Client only, never replicated.** Particles are cosmetic: the server never simulates them, and two clients may show
  slightly different smoke. The emitter itself may be replicated like any component, or a client spawns it when it
  sees an event (an enemy died).
- **Particles sort with the sprites by layer.** An explosion can be under the ships and over the background.
- **Planned after the first version:** sub-emitters ([Sub-emitters](#11-sub-emitters)), then custom modules and
  collisions with the world in a V2 ([Open questions](#14-open-questions)).
- **Not in scope yet:** trails and ribbons, 3D.

## 2. Why the GPU, and why a CPU path too

**The GPU.** Simulating a particle is the same small computation repeated for every particle, with nothing shared
between them: what compute shaders are for. Done on the CPU, every particle is also a vertex sent to the GPU every
frame; done on the GPU, the particles never leave video memory, and the CPU only sends one emitter's parameters per
frame. The renderer is ready for it: Vulkan 1.3, and a `ComputePipeline` class already written for this
(`render/vulkan/pipeline/ComputePipeline.hpp`).

**The CPU as well**, for three reasons:

- **A renderer without compute.** `IRenderer` is meant to be implementable by SFML, SDL or raylib, which have no compute
  shaders. An effect that asks for the GPU must still show on them: the CPU path is its fallback.
- **Gameplay that reads particles.** The CPU cannot read GPU particles back without stalling the frame, so a particle's
  death cannot trigger anything. A few particles the game must know about (a shard that spawns a sound when it dies)
  run on the CPU by choice.
- **A reference.** The CPU simulation is plain C++, unit-tested, and defines what every module of an effect does. The
  GPU shaders are written against it.

This is the split the big engines make (Unreal's Niagara has CPU and GPU emitters, Unity has its CPU Particle System
and its GPU VFX Graph), with one difference: here both paths run the **same effect description**, so moving an effect
from one to the other is a one-field change.

## 3. Where it lives

The renderer carries most of the feature, the simulation and the drawing; the plugin is what links it to the world.
The renderer never includes the ECS, so something must turn entities, transforms and play/stop requests into calls to
`IRenderer`: that is the plugin, as `TextPlugin` is for `IRenderer::draw(Text)`.

```
src/rtype/plugins/backend/graphics/   namespace rtype::plugins::backend::graphics   (target plugins)
├── Particle.hpp                      ParticleSystemId, ParticleSimulation, ParticleInstance, ParticleEmission,
│                                     ParticleDraw
├── ParticleSystemDesc.hpp            the simulation half of an effect: shape, spawn, initial values, forces, curves
├── ParticleCurve.hpp/.cpp            Curve, Gradient and their baking into lookup tables, shared by CPU and GPU
├── ParticleRandom.hpp                the hashed random numbers, shared by CPU and GPU
└── IRenderer.hpp                     + the particle calls (create, destroy, simulate, draw)

src/rtype/plugins/particle/           namespace rtype::plugins::particle               (target plugins)
├── ParticlePlugin.hpp/.cpp           the plugin: registers, inserts the resources, adds the systems
├── ParticleEffect.hpp                an effect: a ParticleSystemDesc plus how its particles look
├── ParticleEffectAssets.hpp/.cpp     the effects by AssetId (resource), with the file loaders by extension
├── IParticleEffectLoader.hpp         what a file format implements to build an effect (Luau later)
├── ParticleEmitter.hpp               an effect played at an entity (component)
├── ParticleEvents.hpp                SpawnParticles, ParticlesFinished
├── ParticleSystems.hpp/.cpp          each emitter's live state, CPU pool or GPU id (resource)
├── CpuSimulation.hpp/.cpp            the CPU simulation of a ParticleSystemDesc
└── exceptions/ParticleExceptions.hpp ParticleLoadException

src/rtype/render/vulkan/particle/     namespace rtype::render::vulkan::particle        (target render-vulkan)
├── GpuParticleSystem.hpp/.cpp        one GPU system: its buffers and lookup tables
├── ParticlePass.hpp/.cpp             records emit, simulate (and later sort) for every system of the frame
└── shaders: particle_emit.comp, particle_simulate.comp, particle.vert (sprite.frag is reused)
```

- **The contract types go in `plugins/backend/graphics/`,** next to `Sprite` and `Text`, never in `render/vulkan/`:
  gameplay must not depend on Vulkan, and another backend must be able to implement them.
- **`ParticlePlugin` is added by the `BackendPlugin`,** as the sprite, tilemap and text plugins are. Its client Render
  system is ordered after `rtype.backend.camera` and before `rtype.backend.sprites`.
- **Dependencies do not change:** `plugins` still knows nothing of Vulkan or Luau, `render-vulkan` still knows nothing
  of the ECS.

## 4. Effects: `ParticleEffect`

An effect is split in two, because the renderer and the plugin do not need the same half:

```cpp
namespace graphics {
/// @brief How the particles of an effect spawn, move and age: what a simulation (CPU or GPU) runs.
struct ParticleSystemDesc {
    ParticleSimulation simulation{ParticleSimulation::kAuto};  ///< Where it runs (see "Choosing CPU or GPU").
    std::uint32_t capacity{1024};                              ///< Most particles alive at once.
    ParticleSpace space{ParticleSpace::kWorld};                ///< kWorld: left behind; kLocal: follow the emitter.

    // Spawn
    float rate{0.0F};                    ///< Particles per second while playing.
    std::vector<ParticleBurst> bursts;   ///< {time, count, cycles, interval}: explosions.
    float duration{1.0F};                ///< Seconds of one cycle.
    bool loop{true};                     ///< Restarts after duration; else plays once.

    // Shape and initial values
    EmitterShape shape;                  ///< Where particles spawn: Point, Circle{radius, edgeOnly}, Rect{size}.
    Range lifetime{1.0F, 1.0F};          ///< Seconds, picked per particle between min and max.
    Range speed{0.0F, 0.0F};             ///< World units per second, along the angle.
    Range angle{0.0F, 2.0F * pi};        ///< Which way particles go, in radians: a cone is a range of angles.
    Range rotation{0.0F, 0.0F};          ///< Radians.
    Range angularVelocity{0.0F, 0.0F};   ///< Radians per second.
    Range size{1.0F, 1.0F};              ///< Scale of the particle's sprite.
    Color color{1.0F};                   ///< Multiplied by the colour over life.

    // Forces
    glm::vec2 gravity{0.0F};             ///< World units per second squared.
    float drag{0.0F};                    ///< Fraction of the velocity lost per second.

    // Over life (0 at birth, 1 at death)
    Gradient colorOverLife;              ///< Colour and alpha keys.
    Curve sizeOverLife;                  ///< Size multiplier keys.
    Curve speedOverLife;                 ///< Speed multiplier keys.
};
}  // namespace graphics

/// @brief An effect: how its particles behave (desc), and how they look.
struct ParticleEffect {
    graphics::ParticleSystemDesc desc;
    core::asset::AssetId texture;        ///< From TextureAssets::load(), or an atlas's texture.
    graphics::math::Rect source;         ///< Part of the texture to show; the whole texture if empty.
    graphics::BlendMode blend{graphics::BlendMode::kAdditive};
    ParticleAlignment alignment{ParticleAlignment::kFixed};  ///< kFixed, or kVelocity: turned and stretched along it.
};
```

- **The renderer only gets the `ParticleSystemDesc`** (what it simulates); the texture, blend and alignment are given
  with each draw, as a `Sprite` carries its texture. The texture is an `AssetId` in the effect, resolved to a
  `TextureId` by the plugin, as for `SpriteRenderer`.
- **Curves and gradients are keys** (`{t, value}`, up to a few per curve), baked into lookup tables of a fixed size by
  `bakeCurve()` / `bakeGradient()` in `ParticleCurve.hpp`. The CPU samples the baked tables; the GPU reads the same
  tables from a 1D texture. Baking happens once, when the effect is created.
- **The shape says where particles spawn, `angle` which way they go.** An arc or a cone is a shape and a range of
  angles, so the direction is never described twice.
- **The modules are fixed fields, not a graph,** for the first version: shape, spawn, initial values, forces, curves.
  New ones (noise, attractors, atlas frames over life) are added as new fields, to both simulations at once
  ([Keeping both paths alike](#10-keeping-both-paths-alike)).

## 5. The renderer contract

`IRenderer` gets four calls, all with a default so that a backend without particles still builds:

```cpp
class IRenderer {
    /// @return Whether createParticleSystem() is implemented: false, the default, means every effect runs on the CPU.
    [[nodiscard]] virtual bool supportsGpuParticles() const { return false; }

    /// @brief Creates a GPU particle system for @p desc: buffers sized by its capacity, its baked curves.
    /// @throws exceptions::UnsupportedFeatureException If supportsGpuParticles() is false (the default).
    [[nodiscard]] virtual ParticleSystemId createParticleSystem(const ParticleSystemDesc& desc);

    /// @brief Destroys a GPU particle system. Safe to call while the backend may still be simulating it.
    virtual void destroy(ParticleSystemId system) {}

    /// @brief Advances a GPU particle system by one frame: emits @p emission.spawnCount particles at the emitter's
    /// transform, then moves, ages and kills them. Between beginFrame() and endFrame(), once per system per frame.
    virtual void simulate(ParticleSystemId system, const ParticleEmission& emission) {}

    /// @brief Draws the particles of a GPU system, or instances simulated by the CPU, in the order of the sprites.
    /// @details The default draws each CPU instance as a Sprite (any backend shows CPU particles for free) and ignores
    /// GPU systems, which it cannot have created.
    virtual void draw(const ParticleDraw& draw);
};
```

```cpp
/// @brief One frame of an emitter, computed by the CPU: what the GPU needs to emit and move its particles.
struct ParticleEmission {
    float deltaTime{0.0F};
    std::uint32_t spawnCount{0};  ///< Particles to emit this frame (rate and bursts, accumulated by the plugin).
    glm::vec2 position{0.0F};     ///< The emitter's GlobalTransform: where particles spawn, or the kLocal space.
    float rotation{0.0F};         ///< Of the GlobalTransform, in radians, clockwise on screen: turns the shape.
    glm::vec2 scale{1.0F};        ///< Of the GlobalTransform: scales the shape.
    std::uint32_t seed{0};        ///< The emitter's seed; the random numbers of a particle depend on it.
    std::uint32_t frame{0};       ///< Frames since the emitter started, so each frame draws new random numbers.
};

/// @brief What to draw: the particles of a GPU system, or the instances of a CPU one.
struct ParticleDraw {
    std::variant<ParticleSystemId, std::span<const ParticleInstance>> particles;
    TextureId texture;
    math::Rect source;
    BlendMode blend{BlendMode::kAdditive};
    ParticleAlignment alignment{ParticleAlignment::kFixed};
};

/// @brief One particle as it is drawn: what a CPU simulation hands the renderer, and what a GPU one keeps in its
/// buffers. The same layout on both sides, so the same vertex shader draws both.
struct ParticleInstance {
    glm::vec2 position;
    glm::vec2 velocity;   ///< For kVelocity alignment.
    float rotation;
    float size;
    Color color;
};
```

- **The CPU decides how many particles spawn, the GPU spawns them.** Counting needs only the emitter's clock (rate,
  bursts, duration, play/stop), which gameplay controls: it stays on the CPU, in the plugin, so both paths spawn the same
  number of particles at the same frames.
- **`draw(ParticleDraw)` goes through the `SpriteQueue`,** whose `Item` becomes `std::variant<Sprite, Text,
  ParticleDraw>`. The backend plugin sorts it by layer with the sprites, so particles are drawn between the right
  sprites. For CPU instances, the span must stay valid until `rtype.backend.sprites` has drawn the queue, in the same
  frame: it points into the emitter's pool, which nothing changes between the plugin's Render system and that one. The
  renderer copies the instances in `draw()`, as it copies sprites.

## 6. Choosing CPU or GPU

```cpp
enum class ParticleSimulation : std::uint8_t {
    kAuto,  ///< GPU if the renderer can, else CPU.
    kGpu,   ///< GPU wanted: falls back to CPU, with a warning, when the renderer cannot.
    kCpu,   ///< Always CPU: few particles that gameplay must read, or for debugging.
};
```

- **The choice is in the effect** (`ParticleSystemDesc::simulation`); a `ParticleEmitter` can override it for one
  emitter. There is no global setting forcing every effect to one path: the effect and the emitter are enough.
- **The fallback is decided when the emitter starts:** for `kAuto` or `kGpu`, the plugin calls `createParticleSystem()`
  if `supportsGpuParticles()` is true; if it is false, or the creation throws, the emitter runs on the CPU for its whole
  life. `kGpu` prints a warning once per effect, `kAuto` stays silent. Gameplay sees no difference.
- **An emitter never moves between paths while it plays:** its particles would vanish or jump.

## 7. The plugin: resources, components, events, systems

```cpp
/// @brief Plays an effect at its entity's GlobalTransform, as the component "rtype.ParticleEmitter".
struct ParticleEmitter {
    core::asset::AssetId effect;      ///< From ParticleEffectAssets::load() or add().
    bool playing{true};               ///< false stops emitting; living particles finish their life.
    std::int32_t layer{0};            ///< Draw order among the sprites.
    float rateScale{1.0F};            ///< Multiplies the effect's rate: more smoke as a ship gets damaged.
    std::optional<graphics::ParticleSimulation> simulation;  ///< Overrides the effect's choice.
    std::uint32_t seed{0};            ///< 0: picked when it starts.
};
```

- **`ParticleEffectAssets`** (resource `rtype.ParticleEffectAssets`) keeps the effects by `AssetId`: `add(name,
  effect)` for an effect built in C++, `load(path)` for a file, read by the loader registered for its extension
  ([Luau](#12-luau)).
- **`ParticleSystems`** (resource `rtype.ParticleSystems`) keeps each emitter's live state, by entity: its clock and
  spawn accumulator, and either its CPU pool or its `ParticleSystemId`. It is a resource rather than a component
  because the ECS has no removal detection yet: a system sweeps it every frame and frees the state of the entities
  whose emitter is gone (destroying their GPU system). Observers ([`docs/ecs/features/16-hooks-and-observers.md`](../ecs/features/16-hooks-and-observers.md))
  will replace the sweep.
- **Events:** `SpawnParticles{effect, position, layer}` plays a one-shot effect at a position without an entity (the
  plugin spawns a short-lived one), for explosions in one line; `ParticlesFinished{entity}` is sent when a one-shot
  emitter has emitted everything and its last particle died (always known on the CPU; on the GPU, when its duration
  plus the longest lifetime has passed, without reading anything back).
- **Systems,** all client-side (`ecs::Side::kClient`):
    - `PostUpdate`, after the transform plugin: start and sweep emitters, advance their clocks, compute each one's
      spawn count, and run the CPU simulation of the CPU emitters.
    - `Render`, after `rtype.backend.camera` and before `rtype.backend.sprites`: call `simulate()` for each GPU
      emitter, and push a `ParticleDraw` per emitter to the `SpriteQueue`, skipped when the emitter's bounds are
      outside the `VisibleArea`.

## 8. The CPU simulation

- **A pool per emitter:** a `std::vector<ParticleInstance>` plus each particle's age, lifetime and random seed, of the
  effect's capacity at most. A dead particle is removed by swapping the last one into its place: the live particles
  stay packed and in a span the renderer can copy at once.
- **One frame:** emit the spawn count (initial values from the shape and the ranges), then for each particle apply the
  forces, integrate, age it, sample its curves at `age / lifetime`, and remove it at the end of its life.
- **Plain C++ without the ECS** (`CpuSimulation`), so it is unit-tested directly and the GPU shaders can be checked
  against it.

## 9. The GPU simulation (Vulkan)

### Buffers, per system

Allocated once at `createParticleSystem()`, GPU-only, **not duplicated per frame in flight**: every frame's commands
run in order on the graphics queue, so a frame only starts on the GPU when the previous one is done with them. Only the
`ParticleEmission` of each frame goes through per-frame memory (push constants).

| Buffer | Content |
|---|---|
| `particles` | One `ParticleInstance` plus age, lifetime and seed per slot, `capacity` slots |
| `deadList` + counter | Indices of the free slots; full at creation |
| `aliveA`, `aliveB` + counters | Indices of the live slots, read from one and written to the other, swapped each frame |
| `indirectArgs` | The `VkDrawIndirectCommand` of the draw, written by the GPU |
| curve tables | The baked curves and gradients of the desc, as a small texture |

### Passes, per frame

1. **Emit:** `spawnCount` threads, clamped to the dead counter. Each pops a free slot with an atomic and fills it from
   the shape and the ranges, with the shared random numbers ([Keeping both paths alike](#10-keeping-both-paths-alike)).
2. **Simulate:** one thread per particle of `aliveA`: forces, integration, ageing. A particle at the end of its life
   goes back to `deadList`, a live one is appended to `aliveB`. The last thread to finish writes the alive count into
   `indirectArgs` (or a one-thread dispatch does).
3. **Sort** (later): a bitonic sort of `aliveB` by age, for `kAlpha` effects only, where the order of overlapping
   particles shows. Additive particles look the same in any order.
4. **Barrier** (synchronization2): compute writes → `DRAW_INDIRECT` and `VERTEX_SHADER` reads.
5. **Draw:** `vkCmdDrawIndirect`, at the system's place among the sprites. `particle.vert` reads the particle through
   `aliveB` (vertex pulling, no vertex buffer), builds its quad, turned and stretched along its velocity for
   `kVelocity`; `sprite.frag` shades it, with the premultiplied blend modes of the sprites.

CPU instances are drawn by the same `particle.vert`, from a per-frame buffer the renderer copies them into, without
the alive list: both paths look the same on screen.

### What changes in `VulkanRenderer`

- **Rendering starts in `endFrame()`, not `beginFrame()`.** `beginFrame()` calls `beginRendering()` today, and a
  compute dispatch cannot be recorded inside a rendering scope. Every draw is already recorded in `endFrame()`, so
  `beginRendering()` moves there, after `ParticlePass` has recorded the emit and simulate dispatches of the frame.
- **The sprite pass records a draw list, not only batches.** A `ParticleDraw` between two sprites splits the batch at
  that point; the pass records sprite batches and particle draws in the order they came.
- **Later:** one large particle buffer for every system with a range each, so that the emit and simulate passes are one
  dispatch for all systems instead of one per system; async compute on a dedicated queue.

## 10. Keeping both paths alike

Each module of an effect is written twice, in C++ and in GLSL. To keep an effect looking the same on both:

- **The same random numbers.** `ParticleRandom.hpp` and `particle_random.glsl` implement the same integer hash (PCG)
  of `(seed, particle index, stream)`; integers hash identically on both sides, so a particle gets the same random
  lifetime, speed and angle. Floating-point results may differ in the last bits; the look does not.
- **The same curves:** both sample the tables `bakeCurve()` baked, with the same interpolation.
- **A module is done when it is in both.** A pull request adding a module to the desc adds it to `CpuSimulation` and to
  the shaders, with a CPU test; otherwise the fallback silently drops it.
- **A test compares them** where a GPU is available (an example or a test run by hand): the same effect, the same seed,
  the alive count and the average position after N frames.

## 11. Sub-emitters

A sub-emitter is an effect whose particles are spawned by the particles of another: sparks that leave smoke where they
die, a firework shell that bursts at the end of its life. Planned after the first version, if it stays as simple as
below.

```cpp
/// @brief An effect spawned by the particles of another, at their position.
struct SubEmitter {
    SubEmitterTrigger trigger{SubEmitterTrigger::kDeath};  ///< kBirth or kDeath (kCollision with collisions).
    core::asset::AssetId effect;   ///< The child effect; it cannot have sub-emitters of its own.
    std::uint32_t count{1};        ///< Child particles spawned per trigger.
    float inheritVelocity{0.0F};   ///< Fraction of the parent particle's velocity given to the children.
    bool inheritColor{false};      ///< The children start with the parent particle's colour.
};
```

- **In the effect:** `ParticleEffect::subEmitters`, a list. The child effect is created with its parent, as its own
  system with its own capacity, texture and blend, and is drawn as its own `ParticleDraw`, right after its parent.
- **On the same path as its parent:** the whole tree runs on the CPU or on the GPU, never split, so a trigger never
  crosses between them.
- **One level only:** a child cannot have sub-emitters. That bounds the passes of a frame and rules out cycles.
- **The CPU path is direct:** when a particle is born or dies, the parent's simulation appends a spawn request
  (position, velocity, colour) to its child, whose next emit consumes them instead of its shape.
- **The GPU path adds one buffer and an indirect dispatch:** the parent's simulate pass appends spawn requests to a
  buffer with an atomic counter (requests beyond the buffer's size are dropped), a one-thread pass turns the counter
  into dispatch arguments, and the child's emit pass runs with `vkCmdDispatchIndirect`, reading the requests. The CPU
  never learns how many children spawned, and does not need to. In the contract, the children are created first and the
  parent is created with links to their `ParticleSystemId`s.
- **`ParticlesFinished`** waits for the children too.

The cost is moderate: the CPU side is a few lines, the GPU side one more buffer, one small pass and one barrier per
parent. Sub-emitters on collision come with collisions.

## 12. Luau

Effects will be written in Luau files, as the project settings are. The Luau is run **once, when the effect is
loaded**, and builds a `ParticleEffect`; it never runs per particle (the particles are on the GPU, where no Luau VM
runs). Gameplay scripts drive emitters (play, stop, `rateScale`) like any component.

`plugins` does not depend on `luau`, so the Luau loader lives outside it:

- `ParticleEffectAssets` has a registry of loaders by file extension (`IParticleEffectLoader`), filled by the
  executable, as `BackendRegistry` and `AudioRegistry` are.
- Some code that depends on both `plugins` and `luau` registers the `.luau` loader, as `replication` is the only code
  that knows both the ECS and the network. Its name and place are chosen when it is written; all this design commits to
  is that the plugin needs no change to accept it.

Until then, effects are built in C++ with `ParticleEffectAssets::add()`.

## 13. Implementation plan

Each step builds and is tested on its own, in this order:

1. **The contract and the CPU simulation:** the types of `plugins/backend/graphics/` (`Particle.hpp`,
   `ParticleSystemDesc`, `ParticleCurve`, `ParticleRandom`), the `IRenderer` calls with their defaults, and
   `plugins/particle/` with `CpuSimulation`, `ParticleEffectAssets`, `ParticleEmitter`, `ParticleSystems` and the
   events, unit-tested (`tests/plugins/`). Effects in C++.
2. **Vulkan draws CPU instances:** `particle.vert`, the per-frame instance buffer. Particles work from end to end; an
   example (`examples/plugins/particle/ParticleBasics`) shows them.
3. **Layers:** `ParticleDraw` in the `SpriteQueue`, the draw list of the sprite pass.
4. **Vulkan simulates:** `beginRendering()` moved to `endFrame()`, `GpuParticleSystem`, `ParticlePass`, the emit and
   simulate shaders, draw indirect, `supportsGpuParticles()`; the fallback logic in the plugin.
5. **Richer effects, on both paths:** curves and gradients in lookup tables, `kVelocity` alignment, sorting for
   `kAlpha`, atlas frames over life.
6. **Sub-emitters,** on birth and death, CPU then GPU.
7. **Luau:** the loader registry, then the Luau loader, once the Luau module is ready.
8. **Docs:** `CLAUDE.md` (structure), `docs/Backend.md` (the new `IRenderer` calls), a user guide like
   `docs/Sprites.md`.

V2: custom modules and collisions with the world ([Open questions](#14-open-questions)).

## 14. Open questions

Settled:

1. **Simulation on the GPU, in the renderer,** behind `IRenderer`; the plugin only links it to the world.
2. **A thin plugin, `plugins/particle/`,** rather than putting the emitter in `backend/graphics/` and its systems in the
   `BackendPlugin`.
3. **A CPU path, chosen per effect or emitter, and the fallback** of an effect that asks for the GPU on a renderer that
   cannot simulate it.
4. **Effects in C++ first,** Luau later through a loader registered from outside `plugins`; the plugin is designed so
   that it can accept one, and the name and place of that code are chosen when it is written.
5. **No global setting:** the CPU/GPU choice per effect and per emitter is enough.
6. **No global budget:** each effect has its own capacity, and that is the only limit.
7. **Sub-emitters** are planned after the first version, as described in [Sub-emitters](#11-sub-emitters).
8. **Custom modules are planned for a V2:** per-particle logic written by the user, as a GLSL module compiled into the
   simulate shader with its C++ twin for the CPU path. To design once the fixed modules have been used.
9. **Collisions with the world are planned for a V2.** A particle that touches a wall or a ship reacts, as its effect
   chooses: it bounces, sticks, or dies (and may trigger a sub-emitter). They are purely visual: particles never push
   anything, and gameplay is never told. Not with Jolt: a query per particle is too heavy, and the GPU cannot call it.
   Instead, both simulations read:
    - for the static world, a **distance field**: a texture over the level, built once from the tilemap's solid layers,
      where each texel holds the distance to the nearest solid surface (negative inside). A particle reads it at its
      position: near 0, it touches; the difference with the neighbouring texels gives the wall's normal, to bounce
      along. Moving by at most the distance read also keeps fast particles from crossing thin walls;
    - for what moves (ships, enemies), a short list of circles and boxes taken from their `Collider`s each frame,
      tested with their exact distance formulas.

   Optional per effect. The static field first, the moving shapes after.

No question is open.
