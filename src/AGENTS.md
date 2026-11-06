# src/ — C++ engine code

Read the root `AGENTS.md` first, then the `AGENTS.md` of the module you work in. The conventions are in
[`docs/CodeStyle.md`](../docs/CodeStyle.md) (Google C++ Style Guide with the project's changes); `.clang-format` and
`.clang-tidy` enforce the mechanical part, so run the tools instead of formatting by hand.

## Layout

- `rtype/<module>/` holds one module each, with its own `xmake.lua` (included from the root `xmake.lua`). Sources are
  found by glob in the module's folder, so a new source file needs no build-file change; a new dependency goes through
  `add_requires` in the module's `xmake.lua` (a package several modules use is declared once in the root).
- Inside a module, one folder per feature, created only when a file goes in it.
- Tests are in `tests/<module>/`, never next to the sources.

## Rules that apply everywhere

- Includes: a sibling by its bare name, anything else from this folder as `rtype/<module>/...`. Never `../`.
- Namespaces mirror the folder path under `rtype/`, except the exceptions a module's `AGENTS.md` states.
- Every shared library marks what it exports with its own `RTYPE_<LIB>_API` macro; a type with no code of its own
  (an interface, an exception) uses the header-only macro of `core`.
- Failures a correct program can hit are returned as results (`luau`, `scripting`); misuse and construction failures
  throw exceptions derived from `std::runtime_error`, kept in an `exceptions/` folder next to the code that throws them.
- A third-party package is private to the module that wraps it: no public header includes it.
- No global mutable state; no `using namespace`.
- Keep the code portable: Linux, macOS, Windows; Clang, Apple Clang, MSVC. Avoid compiler extensions; guard the
  unavoidable ones with a preprocessor check.
- Every source file starts with the Epitech header and every header with `#pragma once` (see the code style).

## Before you finish

Run the `verify` skill. A new engine feature that scripts can use also needs its engine call, its manual page and a
script test (see the `add-engine-call` skill).
