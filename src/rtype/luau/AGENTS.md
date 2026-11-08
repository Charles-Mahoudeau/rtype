# luau — Luau runtime wrapper

A safe C++ wrapper over the Luau VM: state, tables, functions, userdata and type definitions. Static library `luau`,
linked into the executable.

## Boundaries

- Free of the engine: no ECS, no plugin, no game concept. `scripting` builds on it, not the other way.
- The Luau VM is a third-party library private to this module.

## Invariants

- Failures a correct program can hit (a script error, a wrong type, a missing global) return a result; exceptions are
  only for API misuse. See `docs/luau/error-handling.md`.
- A Luau error never crosses the C++ boundary as a raw error: it is caught there and returned.
- Values and references into the VM are valid only while their state lives; the wrapper owns that lifetime.

## Tests and docs

- Tests: `tests/luau/` (`luau-tests`).
- Docs: `docs/luau/error-handling.md`, `docs/luau/type-definitions.md`, `docs/luau/sandbox.md`, `docs/luau/modules.md`.
