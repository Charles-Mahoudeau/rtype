# AGENTS.md

Instructions for AI agents (Claude Code, Codex) working in this repository. `CLAUDE.md` is a symlink to this file, and
each folder with its own instructions has the same pair.

**Before working in a folder, read the `AGENTS.md` files on the path from the root to it** (for `src/rtype/ecs/`: this
file, `src/AGENTS.md`, `src/rtype/ecs/AGENTS.md`). Codex loads only the files above its working directory, not those of
the files it opens, so do it by hand.

## What this is

R-Type is an Epitech Tek3 project: a game engine (C++23, `xmake`, clang) with a Vulkan renderer, Luau scripting,
physics, audio and a replicated client/server network model. It does not contain a game; it runs one.

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
| `src/rtype/engine/` | `engine` | Runs a game folder: settings, plugins, scripting, replication, reloads (`docs/engine/design.md`) |
| `src/rtype/contract/` | `contract-*` | One interface per kind of backend, with its vocabulary (`docs/Backend.md`) |
| `src/rtype/platform/`, `render/`, `audio/` | `platform-*`, `render-*`, `audio-*` | Backends fulfilling a contract; each wraps one third-party library |
| `src/rtype/interop/` | `interop-*` | What a renderer needs from a platform, per graphics API |
| `src/rtype/plugins/` | `plugin-*` | The engine features as ECS plugins: transform, input, graphics, sprite, physics... (`docs/Plugins.md`) |
| `src/rtype/client/`, `server/` | executables | The generic `r-type_client` and `r-type_server` |
| `examples/` | per example | Standalone examples; `examples/games/` holds the games |
| `tests/<module>/` | `<module>-tests` | GoogleTest, one folder per module |
| `docs/` | | Design docs for engine developers |
| `manual/` | | Game developer manual (Zensical site), published to GitHub Pages |

## Architecture rules

1. **Engine is not game.** A game is a folder (`project.luau`, `scripts/`, `assets/`) written only in Luau and run by
   the generic client and server. The engine never names a game, contains no gameplay, and loads a game only through its
   settings, scripts and assets.
2. **The Luau API is the product.** An engine feature a game needs is not done until a script can use it, it is
   documented in the manual (`manual/`) and a script test covers it.
3. **Data over code.** Plugins, backends, window, network and physics are chosen in `project.luau`, never in a
   game-specific `main`. The C++ games in `examples/games/` are the step before this, not a model.
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
contract-*  <-  platform-*, render-*, audio-*   <-  executables, examples
ecs, contract-*  <-  plugin-*                   <-  executables, examples
luau <-  scripting
network  <-  replication
ecs      <-  replication, scripting
scripting, replication, plugin-transform  <-  engine  <-  executables, examples
```

- `core` depends on no project target; `ecs` on `core` only. `core` is the only static library and keeps no state.
- `network` depends on Asio only (private); `replication` is the only module that knows ECS and network; renderers,
  platforms and `luau` never include its headers.
- A contract depends on `core` and glm only (the renderer contract also on the platform contract). No ECS type in a
  contract.
- Backends link their contract only. They never depend on each other; what a renderer needs from a window goes through
  `interop-*`. A window library, miniaudio and the like never leave their backend folder.
- A plugin depends on `core`, `ecs`, glm, the contracts and the plugins it requires, plus private packages. Never on a
  `platform-*`, `render-*`, `audio-*` or `luau` target.
- `engine` links `core`, `ecs`, `luau`, `scripting`, `replication` and `plugin-transform`, and nothing depends on it but
  the executables, examples and its tests. A program adds the other plugins it uses.

## Commands

`xmake` with the `clang` toolchain and C++23.

- Configure: `xmake f -y` (`-m release` for release). Build all: `xmake build -y`. One target: `xmake build engine`.
- Run (the build copies the executables to the repository root): `./r-type_server` and `./r-type_client`, on `game/` or
  `--game <dir>`; other `--name value` flags reach `project.luau` as `engine.args`.
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
   `manual/` page.

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

In `.agents/skills/` (`.claude/skills/` links to it): `verify`, `commit-split`, `open-pr`, `plan-issues`,
`add-plugin`, `add-component`, `add-engine-call`, `luau-gameplay`.
