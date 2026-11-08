# Headless run and UI capability check (GR5-33)

Checked against `main` at `ca0b15aa`, by reading `src/`, `docs/` and `tests/`. Nothing here was run on a built binary.

## Headless Luau

| Capability | Status | Evidence |
|---|---|---|
| Luau game code with no window or renderer | Supported on the server side: `aylo-runtime --server --game <dir>` registers no backend | `src/rtype/runtime/main.cpp`, `docs/engine/design.md` (Executables) |
| `require()` in game scripts | Supported: string paths, `@scripts`/`@self` aliases, entry points `scripts.main/server/client`, sandbox sealed | `ScriptHost.cpp` (`ModuleLoader`, `sandbox()`), `ScriptResolver`, `tests/scripting/ScriptRequireTests.cpp`, `docs/scripting/plan.md` D10 |
| Stop the process from a script with a code | Supported (GR5-35): a script sends `rtype.AppExit { code }` (registered by `App`), `Engine::run` returns the code of the first one | `ecs/app/App.cpp`, `tests/scripting/ScriptSystemRunTests.cpp` |
| Dedicated test-runner mode (run N frames, exit non-zero on failure) | Supported (GR5-35): `aylo-runtime --ticks <n>` stops with 0 after `n` **frames** (not fixed-timestep ticks: a frame runs `FixedUpdate` zero or several times, so the flag gives no tick count); an `AppExit` wins over the limit and a game sends one with a non-zero code to fail; a hot reload whose build fails ends the run with 84; `--ticks` given twice, or with a value that is not a whole number fitting a `size_t`, is refused before the engine starts. A limit in fixed ticks is **not provided** | `runtime/main.cpp`, `CommandLine.cpp` (`takeFrameLimit`), `Engine::run(maxFrames)` |
| Headless client (client-side systems, no window) | Missing: client mode needs the backends `project.luau` names, and the UI and graphics plugins are `Side::kClient`. UI logic cannot be tested without a window | `UiPlugin.cpp` |

Stale docs: `docs/luau/modules.md` and `docs/luau/sandbox.md` still say `ScriptHost` does not expose `require` yet. The code and `docs/scripting/plan.md` say it does.

## UI plugin (`rtype.Ui`) for the debug panel

| Need | Status |
|---|---|
| Panel (layout, background, borders, scroll) | Supported: flexbox via Yoga, `UiBackground`, `UiScroll` |
| Buttons | Supported: `UiButton`, event `rtype.UiClicked` |
| Toggles | Supported: `UiToggle`, event `rtype.UiValueChanged` |
| Text | Supported: `UiText`, wrapping on word boundaries |
| Text fields | Supported, single line: `UiTextInput`, events `UiTextChanged`/`UiTextSubmitted` |
| Sliders | Supported (horizontal) |
| Keyboard and gamepad focus | Supported |
| Ignoring game input over the panel | Supported: `UiPointer::overUi`, `UiFocus::typing` |
| Selection, clipboard, scrolling inside a field | Missing (documented limit) |
| Scroll bar, vertical slider | Missing (documented limit) |
| Dropdown, tables | Missing: build from nodes |

None of the missing UI items blocks a debug panel. See `docs/Ui.md`.

## Gaps and follow-ups

1. Test-runner mode, or confirmation that `AppExit` is writable from Luau (Gameplay to confirm the scope in GR5-27).
2. Update the stale `require` wording in `docs/luau/`.
3. Headless client (needs a null platform and renderer). Only if Gameplay needs it.
