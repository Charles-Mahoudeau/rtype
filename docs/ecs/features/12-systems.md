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

Until the scheduler (#121) and the App (#123) exist, a system is made from a world and run by hand:

```c++
rtype::ecs::System system{world, "rtype.movement", &movement};   // a function, or a lambda without captures
system.setSide(rtype::ecs::Side::kBoth);                          // kServer by default
system.run();             // builds the parameters, calls the function, advances the change tick
system.flushCommands();   // applies its Commands& (the scheduler: after each phase)
system.getAccess();       // what it reads and writes, for the scheduler
```

| Parameter | The system keeps | Each run gets | Access |
|---|---|---|---|
| `Query<A, const B, ...>` | a `QueryState` (registers its components) | a view with the system's `lastRun` | writes `A`, reads `B` |
| `Res<T>` / `ResMut<T>` | the resource's id | the value (throws `MissingResourceException` if absent) | reads / writes resource `T` |
| `NonSend<T>` / `NonSendMut<T>` | same, registered main-thread only | same | same, and the system must run on the main thread |
| `Commands&` | (its own buffer) | the system's buffer | none: applied at the flush |
| `EventReader<T>` | its `EventCursor` | the events it has not read yet | reads event `T` |
| `EventWriter<T>` | the event's id | a writer to the queue | writes event `T` |

Deduction is done once by templates over the function's parameter types (`SystemParam<P>`): each parameter
type knows what the system keeps for it, how to build it from the world, and what access it adds. Taking
anything else (an `int`, a `Query` by reference, `Commands` by value...) is a compile error listing the
accepted types; so is a lambda with captures (hidden state).

### Runs and change ticks

`run()` reads the world's change tick (`thisRun`), builds the parameters against the system's `lastRun`,
calls the function, then sets `lastRun = thisRun` and **advances the world's change tick by one**. So the
tick counts system runs, not frames, and every run has its own number:

- `Changed<T>` / `Added<T>` and `Res::isChanged()` report what changed since **this system** last ran,
  including changes made earlier in the same frame by other systems;
- a system does not see its own changes on its next run;
- the first run sees everything as added (the initial `lastRun` is the oldest comparable tick);
- if the function throws (a missing resource), the run does not count.

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
