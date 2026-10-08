# 11 · Events

## What it is

Events are **typed messages between systems that do not know each other**: "these two entities
collided", "the window was resized", "a client connected". A system writes events into a queue; other
systems read them later in the same frame or the next.

## Why we need it

- **Decoupling (R4):** the collision system does not need to know about damage, sound or score; each
  listener reads `Collision` events.
- **R1:** platform events (Ethan's `Event` variant) enter the ECS as events.
- **R2:** Luau declares and sends its own events (`game.BossDefeated`).
- **R3:** the network plugins announce `ClientConnected` / `ClientDisconnected` as events.

## How others do it

| ECS | Mechanism |
|---|---|
| Bevy | `Events<T>` resource, double-buffered; `EventWriter<T>` / `EventReader<T>` with per-reader cursors |
| Bevy / flecs | Observers and triggers: callbacks run immediately when an event fires |
| EnTT | `entt::dispatcher` (queued or immediate) |
| Unity DOTS | No built-in events; usually entities or buffers used as messages |

## Our choice

**Double-buffered queues, as Bevy does.** Observers (immediate callbacks) are left for later, see
[Hooks and observers](16-hooks-and-observers.md): queued events keep all logic inside scheduled systems,
in a predictable order.

## How it works

### Double buffering

| Moment | Writers push into | Readers see |
|---|---|---|
| Frame N | buffer B | A (events of N-1) + B (events of N so far) |
| End of frame N | — | swap: A is cleared, B becomes the "previous" buffer |
| Frame N+1 | buffer A (now empty) | B (events of N) + A (events of N+1 so far) |

```mermaid
flowchart LR
    subgraph queue ["Events of T"]
        direction TB
        prev["Previous buffer<br/>events of frame N-1"]
        curr["Current buffer<br/>events of frame N"]
    end
    writer["EventWriter::send<br/>push_back + counter++"] --> curr
    reader["EventReader<br/>cursor = last id read"] --> prev
    reader --> curr
    swap["End of frame: clear previous,<br/>current becomes previous"] -.-> queue
```

An event is therefore readable **during the frame it was sent and the next one**, then dropped. Each
reader keeps a **cursor** (the id of the last event it read), so it sees each event exactly once even
if it runs before the writer in one frame and after it in the next.

**The swap follows the phase that writes the event.** Events written in `FixedUpdate` (collisions,
damage) are swapped once per **fixed tick**, not once per frame: a frame that runs `FixedUpdate` three
times would otherwise mix three ticks of events, or drop some. Writing is a `push_back`, and buffers are
cleared rather than freed, so there is no allocation in steady state.

For a complete example (collision detection producing `Collision` events, step by step, and how to
keep it fast), see [Collisions: From Overlap to Event](../design.md#collisions-from-overlap-to-event)
in the design.

```c++
struct Collision {
    rtype::ecs::Entity a;
    rtype::ecs::Entity b;
};

void detectCollisions(/* queries... */ rtype::ecs::EventWriter<Collision> collisions) {
    collisions.send({.a = bullet, .b = enemy});
}

void applyDamage(rtype::ecs::EventReader<Collision> collisions, rtype::ecs::Query<Health> health) {
    for (const Collision& collision : collisions) {
        // ...
    }
}
```

### Platform events

Ethan's `rtype::engine::Event` is a `std::variant` of `event::Resized`, `event::KeyPressed`,
`event::Closed`... The window plugin's `PreUpdate` system calls `IPlatform::pollEvents()` and sends each
alternative to its own queue (`Events<event::Resized>`, ...). A system interested only in resizes reads
only that queue.

```mermaid
flowchart LR
    platform["IPlatform::pollEvents()<br/>std::vector of Event"] --> forward["WindowPlugin system<br/>(PreUpdate)"]
    forward --> resized["Events of event::Resized"]
    forward --> keys["Events of event::KeyPressed"]
    forward --> input["Input::handleEvent()"]
    resized --> render["RenderPlugin: renderer.resize()"]
```

### From Luau

```luau
app:event("game.BossDefeated", { type = "BossDefeated" })   -- fields described by a Luau type

function BossHealth:run(ctx)
    for entity, health in ctx.query do
        if health.value <= 0 then
            ctx.events:send("game.BossDefeated", { boss = entity })
        end
    end
end
```

### Events and the network

Events are **local** to a world. Something the clients must know (an explosion to display) is either
replicated state (a short-lived entity) or an explicit network message from a plugin; events are not
replicated automatically, so gameplay never depends on whether a message arrived.

## Connections

- Uses: [World](06-world.md), [Resources](10-resources.md) (queues are stored like resources).
- Used by: [Systems](12-systems.md) (`EventReader` / `EventWriter` in access sets),
  [App and plugins](14-app-and-plugins.md) (platform events), [Luau bridge](19-luau-bridge.md),
  [Replication](20-replication.md) (connection events).

## Pitfalls

- **Reading too late.** A system that runs only every few frames (a run condition) misses events older
  than two frames.
- **Using events for state.** "Is the boss dead?" is state (a component or resource); "the boss just
  died" is an event.
- **Order between writer and reader.** If the reader runs before the writer in the same frame, it sees
  the event next frame. Order them in the [scheduler](13-scheduler.md) when one frame of delay matters.

## Open questions

- Should some events be replicated (server → clients) by the network plugin, for cosmetic effects only?
  (Team)
