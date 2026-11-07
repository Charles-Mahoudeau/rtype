---
name: add-component
description: Add a component, resource or event to an engine plugin of the AYLO Engine so that C++, scripts, serialization and replication all see it. Use when asked to "add a component", "add a resource", "add an event", or to give a plugin new data.
---

# Add a component, resource or event

Read `src/rtype/plugins/AGENTS.md`, `src/rtype/ecs/AGENTS.md`, `docs/ecs/features/02-component-registry.md` (the
description of a type), `docs/ecs/features/10-resources.md`, `11-events.md` and `15-reflection-and-serialization.md`,
and `docs/CodeStyle.md`.

## Checklist

1. **Place it.** In the plugin that owns it (`src/rtype/plugins/<name>/`), in the namespace of the plugin. Data several
   unrelated modules need goes to `core`; data a contract needs is plain data in the contract, and its ECS declaration
   stays in the plugin that uses it.
2. **Write the plain struct.** Fields with defaults, documented with `///<`. Reflected fields must be of a type the ECS
   can describe: scalars, `std::array` of them, glm vectors (include the glm reflection header of `plugins/`), entities,
   asset ids (the asset-id reflection header), and strings of the ECS. For another type, add a reflection like the
   existing ones (`plugins/GraphicsReflection.hpp`, the audio reflection) instead of exposing it unreflected.
3. **Declare it** right after the struct, in the same namespace, with the macro of its kind and a stable name in the
   `rtype.` namespace for engine data: `RTYPE_ECS_COMPONENT(Type, "rtype.Name", fields...)`, `RTYPE_ECS_RESOURCE(...)`
   or `RTYPE_ECS_EVENT(...)`. Fields that are not listed are stored but invisible to scripts, serialization and
   replication. A struct with no field is a tag.
4. **Say where it lives.** Server only, client only or both, and whether and how it is replicated (never, on every
   change, or at spawn). The server is authoritative: a client never writes replicated data. Choose the least that is
   true.
5. **Register it** in the plugin's build step, so it exists before systems and scripts use it. A plugin that has a
   component its own requirement needs already requires that plugin.
6. **Make it reachable from Luau.** Registered components are visible to scripts by their name with their reflected
   fields (`engine.ecs.system` queries, `ctx.commands`). Check by writing the script: if it needs something the data
   alone cannot do (load an asset, call a C++ API), it also needs an engine call (skill `add-engine-call`).
7. **Tests.** C++ in `tests/plugins/` (default values, registration, the side it lives on) and a script test in
   `tests/engine/` that reads and writes it from Luau. A component that replicates also gets a test in
   `tests/replication/`.
8. **Docs.** The feature page in `docs/` for its plugin; the component table of the plugin in `manual/` (invoke the `write-manual` skill) (fields, types,
   defaults, side, an example script); `docs/Modules.md` if it is a notable class.
9. `verify`, `commit-split`, `open-pr`.

## Pitfalls

- A name changed later breaks saved scenes and the network manifest: choose it once.
- The layout of a reflected struct is part of the network manifest; adding or reordering fields changes it on both sides
  at once.
- Do not register data from a plugin that does not own it.
