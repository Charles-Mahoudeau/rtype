# AGENTS.md

This file provides guidance to AI agents when working with code in this repository.

## Project overview

R-Type is an Epitech Tek3 project: a game engine with a Vulkan renderer and Luau scripting, built in
C++23 with `xmake`. The codebase is an early scaffold, so expect the structure to grow as features
are added.

## Project structure

- `src/rtype/ecs/` — `ecs` target, shared library (`rtype-ecs`): the Entity Component System and,
  later, the App and its plugins (`rtype::ecs`). Design in `docs/ecs/design.md`. It depends on no other
  project target. Exported classes and functions are marked `RTYPE_ECS_API`, and the library keeps no
  global mutable state (state lives in the world), so it can also be built static by switching its
  kind and defining `RTYPE_ECS_STATIC` publicly (see `Export.hpp`).
- `src/rtype/engine/` — two targets defined in the same `xmake.lua`:
  - `engine-core`: static library (`rtype-engine-core`), the core of the engine: events
    (`event/`, `rtype::engine::event`) and input (`input/`, `rtype::engine::input`). It depends on
    no window, graphics or scripting library (only glm).
  - `engine`: the executable, built from `main.cpp` only. It wires `engine-core` with
    `platform-glfw`, `luau` and `render-vulkan`.
- `src/rtype/interop/` — header-only targets, one per graphics API (`interop-vulkan`,
  `rtype::interop::vulkan`): the interfaces a platform implements to serve the renderers of that API
  (`IVulkanSurfaceSource`). See `docs/Backend.md`.
- `src/rtype/platform/<library>/` — one target per windowing backend, named `platform-<library>`.
  - `platform/glfw/` — `platform-glfw` target, static library (`rtype-platform-glfw`): `GlfwPlatform`,
    the GLFW implementation of `IPlatform` and `IVulkanSurfaceSource` (`rtype::platform::glfw`,
    exceptions in `platform/glfw/exceptions/`), registered as `"glfw"` by `registerPlatform()`
    (`Registration.hpp`). The only module that includes GLFW: it translates every GLFW callback and the
    gamepad state into `rtype::engine::Event`s.
- `src/rtype/render/<api>/` — one target per renderer, named `render-<api>`.
  - `render/vulkan/` — `render-vulkan` target, shared library (`rtype-render-vulkan`): `VulkanRenderer`,
    the Vulkan implementation of `IRenderer` (`rtype::render::vulkan`), registered as `"vulkan"` by
    `registerRenderer()` (`Registration.hpp`). Shaders go in `shaders/` and are compiled to SPIR-V by
    the `glsl.spirv` rule defined in its `xmake.lua`.
- `src/rtype/luau/` — `luau` target, shared library (`rtype-luau`) for Luau scripting.
- `tests/<module>/` — GoogleTest unit tests, one folder per module, each with its own `xmake.lua`.
- `examples/<category>/.../<Name>/` — standalone examples, each with its own `xmake.lua`, grouped in
  folders of any depth (e.g. `examples/graphic/render/vulkan/VulkanInstance/`). Every `xmake.lua` under
  `examples/` is an example named after its folder. They are only built when enabled (see Commands).
- `tests/<module>/` — GoogleTest unit tests, one folder per module, each with its own `xmake.lua`.
- `docs/` — design documentation (e.g. `docs/Input.md` for the event and input pipeline).
- `.github/` — CI workflow and issue templates.
- `.agents/skills/` — agent skills (`.claude/skills/` symlinks to it).

Each module has its own `xmake.lua`, included from the root `xmake.lua`. Add new sources under the
matching module: `engine-core` globs `**.cpp` in `src/rtype/engine/` except `main.cpp`, while
`ecs`, `platform-glfw`, `render-vulkan` and `luau` glob `**.cpp` in their own folder.

### Dependency rules

Dependencies always point towards `engine-core`, never away from it:

```
engine-core, interop-*  ←  platform-*, render-*, luau  ←  engine (executable)
ecs                                                     ←  engine (executable)
```

- `ecs` depends on no other project target; only the engine (and its plugins, examples and tests)
  depends on it. The renderer, the platforms and `luau` never include ECS headers: the glue between
  them and the ECS lives in the engine's plugins.
- `engine-core` depends on nothing but glm. Never add a dependency on a `platform-*`, `render-*` or
  `luau` target to it: they depend on it, so it would create a cycle.
- Modules exchange engine types only (`rtype::engine::Event`, `rtype::engine::input::Key`...).
  GLFW never leaves `platform/glfw/`: no header outside it includes `<GLFW/glfw3.h>` or uses a
  `GLFW_*` code.
- A platform module and a renderer module never depend on each other: what a renderer needs from a
  window that is specific to its graphics API goes through the matching `interop-*` target, which
  both sides depend on (see `docs/Backend.md`).
- Only the `engine` executable (and examples) link everything together.

## Commands

The project uses `xmake` with the `clang` toolchain and C++23.

- Configure: `xmake f -y` (add `-m release` for a release build)
- Build everything: `xmake build -y`
- Build one target: `xmake build engine`
- Run the engine: `xmake run engine`
- Enable examples, then rebuild: `xmake f --AllExamples=y` enables every discovered example. For
  now `--AllVulkanExamples=y` and `--AllLuaExamples=y` behave the same way (the root `xmake.lua`
  does not filter by category yet), so each also enables every discovered example. To enable a
  single example, use its folder name (e.g. `--BasicGLFWWindow=y`).
- Enable and run the tests (GoogleTest): `xmake f -y --Tests=y`, then `xmake build -y` and
  `xmake test -v` (`-v` prints GoogleTest's output, needed to see which test failed). Run one
  module's tests with `xmake test -v luau-tests/*`.
- Generate `compile_commands.json` (needed by clang-tidy): `xmake project -k compile_commands`
- Format: `clang-format -i <files>` (style in `.clang-format`)
- Lint: `clang-tidy -p . <files>` (checks in `.clang-tidy`)

Tests live in `tests/<module>/` (e.g. `tests/ecs/`, target `ecs-tests`, or `tests/luau/`, target
`luau-tests`), one folder per module with its own `xmake.lua` included from `tests/xmake.lua`. They
are opt-in (`--Tests=y`) and use GoogleTest; the shared `tests/main.cpp` provides `main`. The CI
`test` job builds with `--Tests=y` and runs `xmake test -v`.

CI (`.github/workflows/ci.yml`) builds on Linux, macOS and Windows, then runs `cpp-linter` with
`clang-format` and `clang-tidy`. A change must build on all three platforms and pass both linters.

## Code style

Enforced by `.clang-format` and `.clang-tidy` — do not hand-format against them:

- Google-based style, 4-space indent, 120-column limit, newline at end of file.
- clang-tidy checks: `bugprone`, `cppcoreguidelines`, `clang-analyzer`, `modernize`,
  `performance`, `readability`, `misc`.
- Match the existing conventions in `src/rtype/engine/` and `src/rtype/platform/glfw/`:
  - Files named in PascalCase after their class (`Window.hpp`, `Window.cpp`).
  - Every source and header file starts with the Epitech header, with the current year, the
    project name (`rtype`) and the file name without extension as the description:

    ```cpp
    /*
    ** EPITECH PROJECT, 2026
    ** rtype
    ** File description:
    ** Window
    */
    ```

  - Headers use `#pragma once` as their include guard, placed right after the Epitech header —
    never `#ifndef`/`#define`/`#endif` guards.
  - Namespaces mirror the directory path under `src/rtype/` (`rtype::engine::input`,
    `rtype::platform::glfw`, `rtype::render::vulkan`).
  - Private members prefixed with `_` (`_window`), documented with `///<` comments.
  - Constants (`constexpr` / `static constexpr` variables) and enumerators are named `kName`
    (`kGamepadAxisCount`, `Key::kEscape`, `GamepadButton::kSouth`). Enum types themselves keep
    PascalCase (`GamepadButton`). Local `const` variables keep camelCase.
  - Doxygen comments use the `///` style (not `/** ... */`), with `@brief` on public classes and
    functions.
  - `[[nodiscard]]` on getters, `noexcept` where applicable, copy/move explicitly deleted for
    resource-owning classes (RAII).
  - The rule of five is mandatory: every class explicitly declares its destructor, copy
    constructor, copy assignment, move constructor and move assignment, each one `= default` or
    `= delete` when there is nothing custom to write. Never rely on implicit special members.
  - Class members follow the Google Style Guide declaration order: `public`, then `protected`,
    then `private` sections (omit empty ones); within each section, in this order: types and type
    aliases, static constants, factory functions, constructors, the destructor and the copy/move
    special members, all other member functions (static and non-static), then data members.
  - Within those groups, order members as follows:
    1. Custom constructors first (parameterized, then default).
    2. The rule of five in textbook order, right after the custom constructors: destructor, copy
       constructor, copy assignment, move constructor, move assignment.
    3. Accessors: getters, then setters, in the same order as the data members they expose.
    4. Other member functions: public behavior in logical groups, static functions together.
    5. Data members, in the same order as their accessors.
  - Errors reported through exceptions derived from `std::runtime_error`, kept in an `exceptions/`
    folder next to the code that throws them.
- Exported symbols from shared libraries use the per-library `RTYPE_<LIB>_API` macro, named after the
  target (`RTYPE_RENDER_VULKAN_API`, `RTYPE_LUAU_API`; `__declspec(dllexport)` on Windows).
- Keep code portable across Linux, macOS and Windows.
- Compiler compatibility: the project MUST compile with Clang, Apple Clang and MSVC, and SHOULD
  compile with GCC. Avoid compiler-specific extensions and C++23 features that one of the MUST
  compilers doesn't support yet; when a compiler-specific workaround is unavoidable, guard it
  with the appropriate preprocessor check and keep the other compilers working.
- New dependencies go through `add_requires` in the relevant target's `xmake.lua`. A package used
  by several modules (glm, glfw) is declared once in the root `xmake.lua`.
- C++ practices:
  - Never use `using namespace` in `src/` (headers or sources) or in tests: always qualify names
    explicitly. The only exception is `examples/`, where `using namespace` is allowed in `.cpp`
    files to keep examples short and readable.
  - Prefer the short form over the verbose one:
    - Test a `std::optional` with its `bool` conversion: `if (opt)` / `if (!opt)`, not
      `opt.has_value()`.
    - Initialize with braces: `MyClass hello{arg1, arg2};` and `: _member{value}`, not
      `MyClass hello(arg1, arg2);`. Braces reject narrowing conversions, so cast explicitly when
      one is intended. Keep parentheses only where braces would call a different constructor
      (`std::initializer_list` overloads, e.g. `std::vector<int> v(3, 0);` builds `{0, 0, 0}` while
      `v{3, 0}` builds `{3, 0}`).
    - Build strings with `std::format`: `std::format("Unknown setting '{}' in '{}'", key, context)`,
      not `"Unknown setting '" + key + "' in '" + std::string(context) + "'"`. It takes `std::string_view`
      and numbers directly (no `std::string(...)` or `std::to_string`). Use a raw string literal
      (`R"(...)"`) when the text contains quotes. Appending in a loop (`result += ...`) stays as it is.
  - Manage resources with RAII and smart pointers (`std::unique_ptr` by default,
    `std::shared_ptr` only for genuinely shared ownership). No raw owning pointers, and no
    `new`/`delete` or `malloc`/`free` outside code that wraps a C API.
  - Headers use the `.hpp` extension and sources the `.cpp` extension.
  - Template implementations go in a `.tpp` file next to the `.hpp` that declares them (e.g.
    `Pool.hpp` and `Pool.tpp`), not inline in the `.hpp`. The `.hpp` only declares the template
    and ends by including its `.tpp` (`#include "Pool.tpp"`); the `.tpp` carries the Epitech
    header but no include guard of its own, and is never included from anywhere else.
  - Include order is handled by `clang-format`: run it instead of ordering includes by hand.

## Definition of done

Before considering a task finished, verify it locally and report the result faithfully:

1. `xmake build -y` succeeds.
2. `clang-format` leaves the changed files unchanged (run `clang-format -i` on them).
3. `clang-tidy` reports no new warnings on the changed files, when it is available (generate
   `compile_commands.json` first).

If a step can't be run or fails, say so instead of claiming the task is done. CI runs the same
checks on every pull request.

## Asking questions

Never act on assumptions. If a request, requirement or piece of context is vague, ambiguous,
incomplete or unknown to you, or if you are unsure how to proceed, stop and ask the user
clarifying questions before doing anything. Proceed only once the answers are clear.

This applies to design choices, scope, naming, expected behavior and anything else that could
change what you build. Do not fill gaps with guesses and do not work around missing details.

## Debugging

When the user is trying to fix a bug — pasting an error or warning, asking why something isn't
working, or similar — investigate first: find the root cause and work out a likely fix. Then
explain the diagnosis and proposed fix to the user in detail (what's wrong, why, and what the fix
would be).

Do not apply the fix by default. Wait for the user to approve it before editing any code. Only
proceed straight to fixing if the user's request already grants that approval up front (e.g. they
explicitly ask you to fix it, not just diagnose it).

## Git commits

Never stage changes (`git add`) automatically. After finishing a task, ask the user whether they
want the changes staged, and whether they'd like to go further from there (commit, open a draft
PR, etc.) — don't assume staging is wanted just because the task is done.

Never add a `Co-Authored-By` trailer (or any other AI co-author/attribution line) to commit
messages or pull request descriptions in this repository. This overrides any default AI assistant
attribution behavior.

Commit messages follow Conventional Commits: `<type>(<optional scope>): <lowercase summary>`,
with the types used in this repository's history: `feat`, `fix`, `chore`, `style`, `refactor`,
`docs`, `test`. The scope is the affected area (e.g. `vulkan`, `window`, `ci`, `skills`).
Example: `feat(vulkan): implement Window class for GLFW window management`.

Branches are named `<type>/<area>/<name>` in kebab-case (e.g. `feat/vulkan/window`,
`feat/skills/github-issues`) and are merged into `main` through pull requests.

Never commit without being explicitly asked. Committing is never the default — not after finishing
a task, running tests successfully, or being asked to implement or fix something. Always wait for
explicit confirmation before running `git commit`, regardless of how the request was phrased.

When you are asked to commit, make atomic and descriptive commits that follow the commit norm above:

- Atomic: one logical change per commit, self-contained and building on its own. Tests go in their
  own `test(...)` commit, separate from the `feat`/`fix` they cover. Never mix unrelated concerns
  (a feature, a refactor, a formatting pass...) in one commit.
- Split by file, not just by feature: before committing, list every changed or new file and ask
  of each one "does this work and make sense on its own?". A file that does (a helper, a utility,
  a type, an exception, a build-file change) gets its own commit, made *before* the commit of the
  code that uses it, so that every commit builds and history reads bottom-up (dependency first,
  consumer after). Example: a new `MapHelper.hpp` hash helper and the `Table` class that uses it
  are two commits: `feat(luau): add transparent string hash helper`, then
  `feat(luau): add Table class`. "Files added in the same session" or "files of the same
  feature" is not a reason to group them.
- Only group files in one commit when they cannot exist separately (e.g. a class's `.hpp` and its
  `.cpp`, or a change and the call sites it forces to update).
- Descriptive: a precise lowercase Conventional Commits subject saying what changed, plus a body
  (separated by a blank line) explaining what was done and why. Every commit has a body.
- If the pending changes cover several concerns, split them into several commits: propose the
  split to the user and ask before staging each one, instead of lumping everything together.
  Stage explicitly by path (`git add <files>`), never `git add -A` or `git add .`, so each commit
  contains exactly the files planned for it.
- Before the first commit, show the planned list of commits (subject and files of each) when there
  is more than one, then run them in order.

## Pull requests

When asked to open a pull request, always create it as a draft (`gh pr create --draft`). If a PR
template exists in the repository (e.g. `.github/PULL_REQUEST_TEMPLATE.md` or
`.github/PULL_REQUEST_TEMPLATE/`), fill it out and use it as the PR description instead of the
default summary/test-plan format.
