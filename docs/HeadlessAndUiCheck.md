# Headless run and UI capability check (GR5-33)

Checked against `main` at `488d0f71`: the source was read, and `aylo-runtime` was built (release) and run headless on a
scratch game (a `main` requiring a helper module, plus one server system). Rows marked "run" were observed on that
binary; the others come from the source only. Not run: the UI (it needs a window and a GPU), and the test suite.

## Headless Luau

| Capability | Status | Evidence |
|---|---|---|
| Luau game code with no window or renderer | Supported on the server side (run): `aylo-runtime <game dir> --server` registers no backend. The game directory is the first argument since #512 | `src/rtype/runtime/main.cpp`, `docs/engine/design.md` (Executables) |
| `require()` in game scripts | Supported (run: `require("./helper")` from `main` returned the module's value): string paths, `@scripts`/`@self` aliases, entry points `scripts.main/server/client`, sandbox sealed | `ScriptHost.cpp` (`ModuleLoader`, `sandbox()`), `ScriptResolver`, `tests/scripting/ScriptRequireTests.cpp`, `docs/scripting/plan.md` D10 |
| Stop the process from a script with a code | **Missing (run)**: a system declaring `events = { send = { "rtype.AppExit" } }` fails with `unknown event 'rtype.AppExit'`. `AppExit` is a C++ event only, so a Luau game cannot end the process or choose an exit code |
| Dedicated test-runner mode (run N ticks, exit non-zero on failure) | Missing: no such flag. It needs `AppExit` exposed to Luau first | `main.cpp` |
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
| Ignoring game input over the panel | Partial. The UI blocks nothing by itself. `UiPointer::overUi` guides pointer-click gating and `UiFocus::typing` says a text field is focused. A focused button leaves both false, and the keyboard or gamepad activation also reaches gameplay. Games must also check `UiFocus::focused` (or `UiInteraction.focused` on the panel's nodes) and decide who owns keyboard and gamepad input; a non-null `focused` is not a guarantee that every focus transition is covered (focus can be dropped by Escape or a click elsewhere in the same frame) |
| Selection, clipboard, scrolling inside a field | Missing (documented limit) |
| Scroll bar, vertical slider | Missing (documented limit) |
| Dropdown, tables | Missing: build from nodes |

None of the missing UI items blocks a debug panel. See `docs/Ui.md`.

## Gaps and follow-ups

1. Expose `AppExit` to Luau, then add a test-runner mode (run N ticks, exit non-zero on failure). Ticket opened.
2. Stale `require`/sandbox wording in `docs/luau/`: fixed in this PR.
3. Headless client with a null platform and renderer: ticket opened, to be scheduled only if Gameplay needs it.
4. Input ownership for the debug panel: documented above. A helper that says "the UI wants the keyboard or gamepad" would remove the guesswork; folded into ticket 1's comment as optional.
