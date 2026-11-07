# ecs — Entity Component System

The ECS, the app, the plugin mechanism, the schedule, deferred commands, the hierarchy, and the engine calls (named
functions that plugins give to scripts). Shared library `ecs`.

## Boundaries

- Depends on `core` only. Never on a contract, a plugin, `luau`, `scripting`, `network` or `replication`.
- No global mutable state: state lives in the world. The library can also be built static, so exports go through its
  export macro.
- Namespaces: the types users write every day stay in the flat `rtype::ecs` whatever their folder; only internal parts
  get a sub-namespace named after their folder (`storage`, `detail`, `exceptions`).

## Invariants

- Data is described once (fields, hooks, a description in the registry) and everything else derives from it: scripts,
  serialization, replication. A new kind of data goes through that description, never around it.
- Every component, resource and event says which side it lives on (server, client or both).
- A system's accesses are declared by its parameters; the scheduler orders and parallelizes from them.
- Structural changes during a frame are deferred commands.
- A plugin names what it requires; the app refuses a plugin whose requirement was not added first.

## Tests and docs

- Tests: `tests/ecs/` (`ecs-tests`).
- Design: `docs/ecs/design.md`; one page per feature in `docs/ecs/features/` (start at its `README.md`); the Luau side
  in `docs/ecs/features/19-luau-bridge.md`.
- Adding a component or an engine call: `add-component` and `add-engine-call` skills.
