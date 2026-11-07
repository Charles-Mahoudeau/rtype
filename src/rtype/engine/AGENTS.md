# engine — runs a game folder

Reads a game folder's `project.luau`, loads the plugin and backend libraries it names, builds the app with them plus the
engine's own, runs it, and rebuilds it on reload. Shared library `engine`.

## Boundaries

- Links `core`, `ecs`, `luau`, `scripting`, `replication`, the transform plugin and the input plugin, and nothing else.
  It links no other plugin, no contract and no backend: it loads them (`plugin/`) from `plugins/` next to the executable
  and from `<game>/plugins/`. A program can still add plugins in code through `EngineConfig::addPlugins`.
- Nothing depends on it except the executables, the examples and its tests.
- It names no game and contains no gameplay. A game reaches the engine only through settings, scripts and assets.

## Invariants

- What a game wants (plugins, backends, window, network, physics) is read from `project.luau`; a new choice goes there,
  not in a code path for one game.
- The command line is generic: the engine reads only what it needs, and every flag reaches `project.luau`.
- A reload rebuilds the app from scratch; nothing survives in globals, except the loaded libraries, which stay until the
  process ends because what they built must outlive every app.
- A library is checked before it is opened: same toolchain (`checkLibrary`), no game library named like an engine one,
  and in a release build a signed one (`PluginSignature`, `docs/Plugins.md#signatures`).
- A missing or invalid game is an error with the path and the reason, not a crash.

## Tests and docs

- Tests: `tests/engine/` (`engine-tests`), which also holds the script-level tests of engine calls and setup forms.
- Docs: `docs/engine/design.md`. The settings a game sees: `manual/`, updated with the `write-manual` skill whenever a setting changes.
- The executable (`src/rtype/runtime/`) only runs the engine; it links no plugin and no backend.
