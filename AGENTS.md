# AGENTS.md

Instructions for AI agents (Claude Code, Codex) working in this repository. `CLAUDE.md` is a symlink to this file, and
each folder with its own instructions has the same pair.

**Before working in a folder, read the `AGENTS.md` files on the path from the root to it** (for `src/rtype/ecs/`: this
file, `src/AGENTS.md`, `src/rtype/ecs/AGENTS.md`). Codex loads only the files above its working directory, not those of
the files it opens, so do it by hand.

## What this is

R-Type is an Epitech Tek3 project: AYLO Engine (Another Yet Lightweight Optimized Engine), a game engine (C++23,
`xmake`, clang) with a Vulkan renderer, Luau scripting, physics, audio and a replicated client/server network model. It
does not contain a game; it runs one.

There are two kinds of work, with different rules:

- **Engine work** (C++, `src/`, `tests/`): read `src/AGENTS.md` and `docs/CodeStyle.md`.
- **Game work** (Luau, `project.luau`, assets, under a `game/` folder): read `examples/games/AGENTS.md`. You never read
  or change C++ for it; if the engine lacks something, file an engine issue.

## Module map

One line per module; the detail of each (classes, files) is in `docs/Modules.md`, and a module with its own
`AGENTS.md` lists it.

| Folder | Target | Responsibility |
|---|---|---|
| `src/rtype/core/` | `core` (static) | What several modules share and none owns: asset ids, hashes, settings, system paths |
| `src/rtype/ecs/` | `ecs` | Entity Component System, the app and its plugin mechanism (`docs/ecs/design.md`) |
| `src/rtype/network/` | `network` | The transport: bytes between machines. Knows no ECS (`docs/network/design.md`) |
| `src/rtype/replication/` | `replication` | World to messages and back: the only code knowing both ECS and network |
| `src/rtype/luau/` | `luau` | Luau runtime wrapper, free of the engine (`docs/luau/`) |
| `src/rtype/scripting/` | `scripting` | The bridge turning what scripts declare into ECS data (`docs/scripting/plan.md`) |
| `src/rtype/engine/` | `engine` | Runs a game folder: settings, loads the plugin and backend libraries, scripting, replication, reloads (`docs/engine/design.md`) |
| `src/rtype/contract/` | `contract-*` | One interface per kind of backend, with its vocabulary (`docs/Backend.md`) |
| `src/rtype/platform/`, `render/`, `audio/` | `platform-*`, `render-*`, `audio-*` | Backends fulfilling a contract; each wraps one third-party library |
| `src/rtype/interop/` | `interop-*` | What a renderer needs from a platform, per graphics API |
| `src/rtype/plugins/` | `plugin-*` | The engine features as ECS plugins, one shared library each, loaded by the engine: transform, input, graphics, sprite, physics... (`docs/Plugins.md`) |
| `src/rtype/editor/` | `editor` | The editor: a static library (and the `aylo-editor` program, always a debug run) that edits a game's scenes (capsules) in docked ImGui panels and plays the game inside them. ImGui is private to it (`docs/Editor.md`) |
| `src/rtype/tools/` | `aylo-sign` | Command line tools; `aylo-sign` makes the keys and signatures of the game plugins (`docs/Plugins.md#signatures`) |
| `src/rtype/runtime/` | `aylo-runtime` | The generic executable that ships a game, a client, or with `--server` the server: it links no plugin and no backend, the engine loads them |
| `examples/` | per example | Standalone examples; `examples/games/` holds the games |
| `tests/<module>/` | `<module>-tests` | GoogleTest, one folder per module |
| `docs/` | | Design docs for engine developers |
| `manual/` | | Game developer manual (Zensical site), published to GitHub Pages |

## Architecture rules

1. **Engine is not game.** A game is a folder (`project.luau`, `scripts/`, `assets/`) written only in Luau and run by
   the generic client and server. The engine never names a game, contains no gameplay, and loads a game only through its
   settings, scripts and assets.
2. **The Luau API is the product.** An engine feature a game needs is not done until a script can use it, it is
   documented in the manual (`manual/`, written with the `write-manual` skill) and a script test covers it. Changing
   what a script sees (a call, component, event, setting, setup form, plugin) means updating the manual in the same
   change: no manual change, no merge.
3. **Data over code.** Plugins (by name, from their libraries), backends, window, network and physics are chosen in
   `project.luau`, never in a game-specific `main`. The C++ games in `examples/games/` are the step before this, not a
   model.
4. **Dependencies point inward** (see below). Contracts know no ECS, backends know only their contract, plugins know no
   backend.
5. **A plugin stands alone.** It owns its data, names the plugins it requires, never adds another, and the plugins form
   a graph without cycles.
6. **The server is authoritative.** Every piece of data says which side it lives on; a client never writes replicated
   data; bytes from the network never crash, throw or assert.
7. **A third-party library stays private** to the one module that wraps it.
8. **No global state.** State lives in the world.
9. **Errors.** A failure a correct program can hit is returned as a result; misuse throws; a script error never takes
   the game down.
10. **Portable.** Linux, macOS and Windows; Clang, Apple Clang and MSVC must compile it (GCC should).

## Dependency rules

```
core <-  ecs, contract-*
contract-*  <-  platform-*, render-*, audio-*   (loaded by the engine, built before the executables)
ecs, contract-*  <-  plugin-*                   (loaded by the engine, built before the executables)
luau <-  scripting
network  <-  replication
ecs      <-  replication, scripting
scripting, replication  <-  engine  <-  executables, examples
```

- `core` depends on no project target; `ecs` on `core` only. `core` is static and keeps no state, so any library may
  link it. `luau`, `network`, `replication`, `scripting` and `engine` are static too, linked into the executable: no
  plugin, backend or contract may link them, since a shared library would get its own copy of their state. The build
  refuses it (rule `rtype.static_modules`). `ecs` and the contracts stay shared: they hold what plugins and backends
  share (registries, type information).
- `network` depends on Asio only (private); `replication` is the only module that knows ECS and network; renderers,
  platforms and `luau` never include its headers.
- A contract depends on `core` and glm only (the renderer contract also on the platform contract). No ECS type in a
  contract.
- Backends link their contract only. They never depend on each other; what a renderer needs from a window goes through
  `interop-*`. A window library, miniaudio and the like never leave their backend folder.
- A plugin depends on `core`, `ecs`, glm, the contracts and the plugins it requires, plus private packages. Never on a
  `platform-*`, `render-*`, `audio-*` or `luau` target.
- `engine` links `core`, `ecs`, `luau`, `scripting`, `replication`, and nothing depends on it but the executables,
  examples and its tests. It links no plugin and no backend: it loads them from their libraries (`plugins/` next to the
  executable, `<game>/plugins/`; `lib/` holds the rest), as `project.luau` names them. A program can
  still add plugins in code through `EngineConfig::addPlugins`.
- The executables link no plugin and no backend; the build only makes them first (`plugin-libraries`,
  `backend-libraries`). A library exports one entry point (`RTYPE_PLUGIN_LIBRARY`, or `RTYPE_LIBRARY` for a backend,
  which knows no ECS): see `docs/Plugins.md#loading`.

## Commands

`xmake` with the `clang` toolchain and C++23.

- Configure: `xmake f -y` (`-m release` for release). Build all: `xmake build -y`. One target: `xmake build engine`.
- Run (the build copies `aylo-runtime` to the repository root): `./aylo-runtime` (client) and `./aylo-runtime --server`,
  on `game/` or `--game <dir>`; other `--name value` flags reach `project.luau` as `engine.args`. `./r-type_server` and
  `./r-type_client` at the root are `sh` wrappers (`.bat` ones on Windows) that start it with the right side (the names
  the subject asks for).
- The editor: `./aylo-editor --game <dir>` (both sides in one world, the scenes edited in panels:
  `docs/Editor.md`).
- Duel example: `xmake f -y --rtype_duel=y`, then `xmake run rtype_duel server [--port <port>]` and
  `xmake run rtype_duel client [--host <host>] [--port <port>]` (or the generic executables with
  `--game examples/games/rtype_duel/game`).
- Examples are opt-in: `xmake f --AllExamples=y`, or one by folder name (`--BasicGLFWWindow=y`, `--EcsTypedQueries=y`).
  `--AllVulkanExamples=y`, `--AllLuauExamples=y`, `--AllEcsExamples=y` currently enable every example too.
- Tests (GoogleTest, opt-in): `xmake f -y --Tests=y`, `xmake build -y`, `xmake test -v` (`-v` shows which test failed).
  One module: `xmake test -v luau-tests/*`.
- `compile_commands.json` (needed by clang-tidy): `xmake project -k compile_commands`.
- Format: `clang-format -i <files>`. Lint: `clang-tidy -p . <files>`. Both on the changed files:
  `.github/scripts/lint.sh <base>`.
- The game manual: `uvx zensical build -f manual/mkdocs.yml`.

CI (`.github/workflows/ci.yml`) builds on Linux, macOS and Windows and lints the changed files. In a stack of pull
requests (`gh stack`), only the top one runs the checks, on the whole stack.

## Definition of done

Before calling a task finished, run the `verify` skill and report the result faithfully:

1. `xmake build -y` succeeds (with `--Tests=y`).
2. `xmake test -v` passes, and new behavior has a test (C++ in `tests/<module>/`, Luau features also a script test).
3. `clang-format` leaves the changed files unchanged and `clang-tidy` reports nothing new
   (`.github/scripts/lint.sh <base>`).
4. The docs of what changed are updated: the `docs/` page of the module, and for anything a script can use, the
   `manual/` page, written with the `write-manual` skill (a pull request that changes the script API and leaves
   `manual/` untouched is not done, unless its description says why no script can see the change).

If a step cannot be run or fails, say so; never claim it done.

## Autonomy

- **Free, never ask:** read, edit, build, test, lint, create local branches.
- **Autonomous runs** (auto mode, or the prompt says so) may also commit, push feature branches, open **draft** pull
  requests and create or edit issues, without asking.
- **Interactive:** "why does X fail" means diagnose and propose a fix; "fix X" means fix it.
- **Ask only for:** a real design fork the rules and docs do not settle (state the options and a recommendation);
  anything destructive or hard to undo (merging, force-pushing a shared branch, deleting branches, issues or labels,
  touching `main`, releases); anything outside this repository.
- When you proceed on an assumption, write it in the pull request description.

## Git, pull requests, issues

- Commits: Conventional Commits `<type>(<scope>): <lowercase summary>` with `feat`, `fix`, `chore`, `style`,
  `refactor`, `docs`, `test`; atomic, a body on every commit, files staged by path, tests in their own `test(...)`
  commit. Details in the `commit-split` skill.
- Branches `<type>/<area>/<name>` in kebab-case, merged into `main` by pull request. Never push to `main`, never merge.
- Pull requests are drafts, from the template, one per issue, stacked with `gh stack` when they depend on each other
  (`open-pr` skill).
- Issues are titled `[Area] <summary>` (`[Area] <ID>: <summary>` for tasks of a plan). Priority, Size and Status live in
  the project "R-Type" (number 8), not in labels; parents and blocked-by links are real GitHub relationships, not text
  (`plan-issues` skill).
- No `Co-Authored-By` or other AI attribution in commits or pull requests.

## Skills

In `.agents/skills/` (`.claude/skills/` links to it). **A skill is not optional background: when a task matches a row,
invoke the skill before doing the task, even if the user did not name it.** Asking for the task is asking for its skill;
doing it by hand from memory skips rules (labels, project fields, template, commit split) that only the skill holds.

| When you... | Invoke |
|---|---|
| open, create or update a pull request, "submit", "push for review" | `open-pr` (which runs `verify`) |
| commit, or write commit messages, or push a branch with several changes | `commit-split` |
| finish a change, or are about to say "done" | `verify` |
| touch anything a script can see: an engine call, a component, an event, a setting, a setup form, a plugin, a scripting-bridge rule; or find a manual page missing or wrong | `write-manual` |
| add an engine call or a setup form | `add-engine-call` (then `write-manual`) |
| add a component, resource or event to a plugin | `add-component` (then `write-manual`) |
| add a plugin | `add-plugin` (then `write-manual`) |
| write or change a game in Luau | `luau-gameplay` |
| file issues, an epic, sub-issues, dependencies | `plan-issues` |

The skills chain: a feature PR goes `add-*` -> `write-manual` -> `verify` -> `commit-split` -> `open-pr`.
