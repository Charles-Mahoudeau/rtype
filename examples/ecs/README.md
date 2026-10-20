# ECS examples

One small console program per feature of the ECS (`src/rtype/ecs`). Each one runs on its own, needs no input and
prints what happens step by step, so reading its output next to its code explains the feature.

## Build and run

```sh
xmake f -y --EcsTypedQueries=y   # one example, by its folder name
xmake f -y --AllEcsExamples=y    # all of them (for now this enables every example of the repository)
xmake build -y EcsTypedQueries
xmake run EcsTypedQueries
```

## The examples

Read them roughly in this order: each one builds on the ones before.

| Example | Shows | Details |
|---|---|---|
| [`EcsEntities`](EcsEntities/src/main.cpp) | spawning and despawning, an index and a generation, a freed slot reused, a stale handle refused | [Entity](../../docs/ecs/features/01-entity.md) |
| [`EcsComponents`](EcsComponents/src/main.cpp) | `RTYPE_ECS_COMPONENT` (fields, tags), add / get / getMut / has / remove, what the registry knows | [Component registry](../../docs/ecs/features/02-component-registry.md) |
| [`EcsRuntimeComponents`](EcsRuntimeComponents/src/main.cpp) | a component described at run time (as the Luau bridge does), read and written as bytes, a wrong description refused | [Component registry](../../docs/ecs/features/02-component-registry.md) |
| [`EcsTypedQueries`](EcsTypedQueries/src/main.cpp) | `Query<...>` terms: `T`, `const T`, `Optional<T>`, `With<T>`, `Without<T>` | [Queries](../../docs/ecs/features/07-queries.md) |
| [`EcsDynamicQueries`](EcsDynamicQueries/src/main.cpp) | `QueryBuilder` by component id, bytes by data term, optional terms, filters, errors | [Queries](../../docs/ecs/features/07-queries.md) |
| [`EcsChangeDetection`](EcsChangeDetection/src/main.cpp) | `Added<T>` / `Changed<T>` since a system's last run, `Mut<T>` marking only on write | [Change detection](../../docs/ecs/features/08-change-detection.md) |
| [`EcsCommands`](EcsCommands/src/main.cpp) | spawn, add, despawn recorded by a system and applied at the flush | [Commands](../../docs/ecs/features/09-commands.md) |
| [`EcsResources`](EcsResources/src/main.cpp) | `Res` / `ResMut`, a main-thread `NonSend` resource, a missing resource | [Resources](../../docs/ecs/features/10-resources.md) |
| [`EcsEvents`](EcsEvents/src/main.cpp) | cursors, an event's two-update lifetime, `EventWriter` / `EventReader` | [Events](../../docs/ecs/features/11-events.md) |
| [`EcsSystems`](EcsSystems/src/main.cpp) | systems from a function and a lambda, their deduced access, side, run and flush | [Systems](../../docs/ecs/features/12-systems.md) |
| [`EcsSchedules`](EcsSchedules/src/main.cpp) | order inside a phase, ambiguities refused, a whole frame, run modes | [Scheduler](../../docs/ecs/features/13-scheduler.md) |
| [`EcsFixedTimestep`](EcsFixedTimestep/src/main.cpp) | ticks per frame for 5 / 16 / 50 ms frames, the cap, the `Time` resource | [Scheduler](../../docs/ecs/features/13-scheduler.md#fixed-timestep) |
| [`EcsAppAndPlugins`](EcsAppAndPlugins/src/main.cpp) | the App, plugins and groups, `MinimalPlugins`, `AppExit` | [App and plugins](../../docs/ecs/features/14-app-and-plugins.md) |
| [`EcsScriptSystems`](EcsScriptSystems/src/main.cpp) | `DynamicQueries` configured with a `QueryList`, a `ScriptCallable` | [Systems](../../docs/ecs/features/12-systems.md#configured-parameters) |

## Conventions

- Each example is `examples/ecs/<Name>/xmake.lua` and `src/main.cpp`; its build option is its folder name.
- `using namespace rtype::ecs;` is allowed here (examples only), to keep them short.
- `main()` only calls `run()` and prints an exception if one escapes, so the examples pass `clang-tidy` like the
  rest of the code.
