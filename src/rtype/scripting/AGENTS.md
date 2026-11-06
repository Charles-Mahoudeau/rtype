# scripting — the Luau bridge

Turns what a script declares (components, systems, resources, events, assets, input) into ECS data, and runs script
systems on the app. Shared library `scripting`. This is the module that makes the Luau API real.

## Boundaries

- Depends on `core`, `ecs` and `luau`. It knows no plugin and no backend, and the plugins never depend on it or on
  `luau`. The scripting plugin is the thin piece that owns the runtime and loads the game's scripts.
- Has its own error and result types, like `luau`: failures a correct script can hit (a typo, a bad field) are results
  turned into script errors; exceptions are for misuse and for a script that cannot be loaded at build time.

## Invariants

- A script error never takes the game down: it is reported and the frame goes on.
- A script touches ECS data only through views checked against the current frame, so a stale view is an error, not
  undefined behavior.
- Script component layouts are deterministic, identical on server and client.
- Anything a script can do is documented in `manual/` and covered by a script test. A new setup form or engine call
  without them is not done.
- Engine calls from scripts go through the generic call mechanism of `ecs`; the bridge never special-cases a plugin.

## Tests and docs

- Tests: `tests/scripting/` (`scripting-tests`). Script-level tests of engine calls, setup forms and physics from Luau
  are in `tests/engine/` (`engine-tests`), where the plugins are available.
- Design and status: `docs/scripting/plan.md`, `docs/ecs/features/19-luau-bridge.md`, `docs/engine/design.md`.
- What exists: `docs/Modules.md`. The user-facing API: `manual/`.
- Skills: `add-engine-call`, `luau-gameplay`.
