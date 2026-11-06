# engine — runs a game folder

Reads a game folder's `project.luau`, builds the app with the plugins the program lists plus the engine's own, runs it,
and rebuilds it on reload. Shared library `engine`.

## Boundaries

- Links `core`, `ecs`, `luau`, `scripting`, `replication` and the transform plugin, and nothing else. It links no other
  plugin and no contract: the program (executable or example) adds what it uses.
- Nothing depends on it except the executables, the examples and its tests.
- It names no game and contains no gameplay. A game reaches the engine only through settings, scripts and assets.

## Invariants

- What a game wants (plugins, backends, window, network, physics) is read from `project.luau`; a new choice goes there,
  not in a code path for one game.
- The command line is generic: the engine reads only what it needs, and every flag reaches `project.luau`.
- A reload rebuilds the app from scratch; nothing survives in globals.
- A missing or invalid game is an error with the path and the reason, not a crash.

## Tests and docs

- Tests: `tests/engine/` (`engine-tests`), which also holds the script-level tests of engine calls and setup forms.
- Docs: `docs/engine/design.md`. The settings a game sees: `manual/`.
- The executables (`src/rtype/client/`, `src/rtype/server/`) only list plugins and backends and run the engine.
