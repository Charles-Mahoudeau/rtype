# 10 · Resources

## What it is

A resource is **a single value owned by the world**, not attached to any entity: the frame time, the
input state, the configuration, the asset storage, the renderer. Systems access resources by type (C++)
or by name (Luau).

## Why we need it

Some data has exactly one instance. Putting it on an entity ("the entity that holds the time") would be
awkward and slow to find.

- **R1 (Bevy-style engine):** every engine service becomes a resource: Ethan's `Backend` (platform +
  renderer), `Input`, `Settings`, `Assets<Texture>`, the network state.
- **R2 (Luau):** scripts read resources like `time` and write their own (a `game.Score` resource).

## How others do it

| ECS | Singletons |
|---|---|
| Bevy | `Res<T>` / `ResMut<T>`; `NonSend<T>` for data that must stay on the main thread |
| flecs | Singletons: a component set on the component's own entity |
| EnTT | `registry.ctx()` context variables |
| Unity DOTS | Singleton components (`SystemAPI.GetSingleton<T>()`) |

## Our choice

Bevy's model: **`Res<T>` (read), `ResMut<T>` (write)** and **`NonSend<T>` / `NonSendMut<T>`** for
main-thread-only values. Resources are registered by name like components, so the type-erased API and
Luau can reach them.

## How it works

```c++
app.insertResource(Time{.fixedDelta = 1.F / 60.F});
app.insertResource(std::move(settings));               // Ethan's engine::config::Settings
app.insertNonSendResource(std::move(backend));         // Ethan's engine::backend::Backend

void tick(rtype::ecs::ResMut<Time> time);
void drawSprites(/* queries... */ rtype::ecs::NonSendMut<rtype::engine::backend::Backend> backend);
```

| Parameter | Access | Thread |
|---|---|---|
| `Res<T>` | read | any |
| `ResMut<T>` | write | any |
| `NonSend<T>` | read | main thread only |
| `NonSendMut<T>` | write | main thread only |

### Why main-thread resources

GLFW requires most of its calls (window creation, event polling) on the main thread, and Vulkan
presentation is usually tied to the window's thread. Marking the platform and renderer as `NonSend`
tells the scheduler that systems using them must run on the main thread. In the sequential v1 scheduler
this changes nothing at runtime; it is there so a parallel scheduler later knows the constraint.

### Resources the engine plugins insert

| Resource | Inserted by | Kind |
|---|---|---|
| `Time` | core | `Res` |
| `Settings` | core (from the config file) | `Res` |
| `Backend` (platform + renderer) | `WindowPlugin` / `RenderPlugin` | `NonSend` |
| `Input` | `InputPlugin` | `Res` |
| `Assets<Texture>`, `Assets<Sound>` | `AssetPlugin` | `Res` |
| `ServerState` / `ClientState` | `ServerPlugin` / `ClientPlugin` | `Res` |
| `State<T>` | `app.state<T>()` (see [Scheduler](13-scheduler.md)) | `Res` |

### From Luau

```luau
app:resource("game.Score", { type = "Score", replicated = true })   -- declared like a component

function ScoreOnKill:run(ctx)
    ctx.resources["game.Score"].value += 100
end
```

A script resource is described by a Luau type exactly like a [component](02-component-registry.md); a
replicated resource is sent like a component of a hidden singleton entity.

## Connections

- Uses: [World](06-world.md), [Component registry](02-component-registry.md) (descriptions, names).
- Used by: [Systems](12-systems.md) (access sets), [Scheduler](13-scheduler.md) (main-thread
  constraint, States), [App and plugins](14-app-and-plugins.md), [Replication](20-replication.md).

## Pitfalls

- **Turning everything into a resource.** Per-player data (score per player) belongs on the player's
  entity, not in a resource map; otherwise queries, replication and despawning do not see it.
- **Hidden ordering.** Two systems writing the same resource must be ordered explicitly; the scheduler
  rejects ambiguous orders (see [Scheduler](13-scheduler.md)).
- **Missing resource.** A system asking for `Res<T>` when nothing inserted `T` is a registration error,
  reported when the schedule is built, not in the middle of a frame.

## Open questions

- Should replicated resources exist, or should replicated singletons always be entities? (ECS + network)
