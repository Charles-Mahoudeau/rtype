# Contributing to the AYLO Engine

Thank you for helping. This page is the short version of how the team works; the detail lives in the pages it links.

## Two kinds of work

| You change | You read first | Language |
|---|---|---|
| The engine (`src/`, `tests/`) | [`docs/`](docs/index.md), [`docs/CodeStyle.md`](docs/CodeStyle.md), the `AGENTS.md` of the folder | C++23 |
| A game (`examples/games/<game>/game/`) | The [game developer manual](https://charles-mahoudeau.github.io/rtype/) | Luau only |

A game never needs C++. If the engine lacks something, open an engine issue instead of working around it.

## Set up

```sh
xmake f -y --Tests=y         # configure, with the tests
xmake build -y               # build everything
xmake test -v                # run the tests
xmake project -k compile_commands   # for clang-tidy and your editor
```

Requirements and platforms are in the [README](README.md#requirements).

## Workflow

1. **An issue first.** Every change starts from an issue titled `[Area] <summary>` (bug or feature template). Its
   priority, size and status live in the GitHub project "R-Type".
2. **A branch per issue**, from `main`, named `<type>/<area>/<name>` in kebab-case: `feat/network/reconnect`,
   `fix/vulkan/swapchain-resize`, `docs/manual/physics`.
3. **Atomic commits** in [Conventional Commits](https://www.conventionalcommits.org/) form, lowercase summary, and a body
   saying what and why:

   ```
   feat(network): resume a session after a short connection loss

   The server keeps a lost session for reconnectGrace seconds and accepts a resume token...
   ```

   Types: `feat`, `fix`, `refactor`, `style`, `docs`, `test`, `chore`. Tests go in their own `test(...)` commit.
4. **A draft pull request** from the template, linked to its issue. Dependent pull requests are stacked (`gh stack`).
5. **Review and CI.** CI builds and tests on Linux, macOS and Windows and lints the changed files. A pull request is
   merged by a reviewer once it is green; nobody pushes to `main`.
6. **Milestones are tagged** on `main` (`v1.0.0` for the first delivery, and so on).

## Definition of done

A change is done when:

- `xmake build -y` and `xmake test -v` pass, and new behavior has a test (`tests/<module>/`; a script feature also a
  script test);
- `clang-format` leaves the changed files unchanged and `clang-tidy` reports nothing new
  (`.github/scripts/lint.sh <base>`);
- the docs are updated: the `docs/` page of the module, and **the manual for anything a script can see** (a call, a
  component, an event, a setting, a plugin). No manual change, no merge.

## Rules worth knowing before you write code

- **Engine is not game**: the engine never names a game or contains gameplay.
- **Dependencies point inward**: contracts know no ECS, backends know only their contract, plugins know no backend.
- **The server is authoritative**: a client never writes replicated data; bytes from the network never crash, throw or
  assert.
- **No global state**: state lives in the world.
- **A third-party library stays private** to the one module that wraps it.
- **Portable**: Linux, macOS and Windows; Clang, Apple Clang and MSVC.

The full list, with reasons, is in [`AGENTS.md`](AGENTS.md#architecture-rules).

## Documentation

| Where | For whom | Built with |
|---|---|---|
| [`manual/`](manual/) | Game developers (Luau) | `uvx zensical build -f manual/mkdocs.yml` |
| [`docs/`](docs/index.md) | Engine developers (C++) | `uvx zensical build -f docs/mkdocs.yml` |

Both are published on GitHub Pages when `main` changes: the manual at
<https://charles-mahoudeau.github.io/rtype/>, the developer documentation under
<https://charles-mahoudeau.github.io/rtype/engine/>. Write in English, short sentences, tables for facts.

## AI agents

The repository is set up for Claude Code and Codex: `AGENTS.md` files (and their `CLAUDE.md` links) give the rules per
folder, and `.agents/skills/` holds the step-by-step procedures (adding a plugin, a component, an engine call, writing
the manual, committing, opening a pull request). Their output follows the same rules and the same review as anyone's.
