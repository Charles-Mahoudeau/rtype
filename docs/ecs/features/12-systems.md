# 12 · Systems

## What it is

A system is **logic**: a function that reads and writes the world through queries, resources, events
and commands. Components hold data; systems hold every behavior of the game (movement, shooting,
damage, spawning waves, drawing).

## Why we need it

- **Principles:** keeping all logic in systems is what makes components replicable, scriptable and
  serializable.
- **R2 (Luau):** most gameplay systems are written in Luau.
- **R4:** declared access lets the [scheduler](13-scheduler.md) order systems, and later run them in
  parallel, without changing them.

## How others do it

| ECS | System style | Access known by the scheduler? |
|---|---|---|
| Bevy | Free function; parameters (`Query`, `Res`, `Commands`...) deduced from the signature | Yes, from the types |
| flecs | `world.system<A, B>().each(callback)` | Yes, from the query |
| Unity DOTS | `ISystem` struct with `OnUpdate` | Yes, from queries |
| Unreal Mass | `UMassProcessor` class with `ConfigureQueries` + `Execute` | Yes |
| EnTT | No systems: plain functions over views | No |

## Our choice

- **C++:** Bevy-style free functions (or lambdas). The parameters are deduced from the signature at
  registration, which gives the system's **access set**.
- **Luau:** **system classes**: a table declaring name, phase, query and options, with a `run` method.
- Both register the same way into the scheduler, with a name, a phase, ordering constraints, an access
  set and a **side** (server, client or both).

## How it works

### C++ systems

```c++
void movement(rtype::ecs::Query<Transform, const Velocity> query, rtype::ecs::Res<Time> time) {
    for (auto [entity, transform, velocity] : query) {
        transform->position += velocity.value * time->fixedDelta;
    }
}

app.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.movement", &movement)
    .side(rtype::ecs::Side::kBoth);
```

| Parameter | Access |
|---|---|
| `Query<A, const B, ...>` | writes `A`, reads `B` |
| `Res<T>` / `ResMut<T>` | reads / writes resource `T` |
| `NonSend<T>` / `NonSendMut<T>` | main-thread resource |
| `Commands&` | deferred structural changes |
| `EventReader<T>` / `EventWriter<T>` | reads / writes event queue `T` |

Deduction is done once by templates over the function's parameter types: each parameter type knows how
to build itself from the world and what access it adds.

### Luau systems

```luau
local ShieldRegen = ecs.system("game.ShieldRegen", {
    phase = "FixedUpdate",                                   -- default
    side = "server",                                         -- default
    query = { write = { "game.Shield" }, without = { "rtype.Dead" } },
    after = { "rtype.movement" },
})

function ShieldRegen:run(ctx)
    for entity, shield in ctx.query do
        shield.strength = math.min(shield.strength + shield.regen * ctx.time.fixedDelta, 100)
    end
end

return ShieldRegen
```

`ctx` gives the query results, `ctx.time`, `ctx.commands`, `ctx.events` and `ctx.resources`. Because a
script cannot be inspected like a C++ signature, its access is **declared** in the table, and the bridge
refuses writes to anything declared as read-only.

### The side rule

| Side | Runs on | May write |
|---|---|---|
| `server` (default) | the server (and standalone) | anything: this is authoritative gameplay |
| `client` | clients (and standalone) | only `kClientOnly` components; anything else would be overwritten by replication |
| `both` | everywhere | components that are simulated identically on both sides (bullet motion for `kSpawnOnly` entities) |

In standalone mode (single player), the App runs server and client systems in one world, so the same
game code works solo and online (see [App and plugins](14-app-and-plugins.md)).

### State

A system keeps **no hidden state**. A cooldown is a component on the weapon; a wave counter is a
resource. That is what lets the server save, replicate or reload everything.

## Connections

- Uses: [Queries](07-queries.md), [Resources](10-resources.md), [Events](11-events.md),
  [Commands](09-commands.md), [Change detection](08-change-detection.md) (`lastRun`).
- Used by: [Scheduler](13-scheduler.md), [App and plugins](14-app-and-plugins.md),
  [Luau bridge](19-luau-bridge.md).

## Pitfalls

- **Static variables in systems.** They survive across worlds and break rooms, tests and hot reload.
  Use resources.
- **Big systems.** A system that does five things blocks ordering and parallelism. Keep one purpose per
  system.
- **Client systems writing gameplay state.** Their writes vanish at the next snapshot; the bridge
  rejects them at registration.

## Open questions

- Should system parameters include `Local<T>` (per-system persistent state, as in Bevy), or is "use a
  resource" enough? (ECS)
