# Code style

The C++ code follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), with the changes
below. Only the differences and the project's own rules are listed here; what `.clang-format` and `.clang-tidy` enforce
is not repeated, so run the tools instead of formatting by hand (`clang-format -i <files>`, `clang-tidy -p . <files>`, or
`.github/scripts/lint.sh <base>` for the changed files). The clang-tidy checks are `bugprone`, `cppcoreguidelines`,
`clang-analyzer`, `modernize`, `performance`, `readability` and `misc`.

This file is for C++ work under `src/`, `tests/` and `examples/`. It is read through `src/AGENTS.md`.

## Changes from Google style

### Layout

- 4-space indent, 120 columns, newline at end of file (`.clang-format`). Include order is also `clang-format`'s: run it
  instead of ordering includes by hand.
- Exceptions are used. Errors are reported through exceptions derived from `std::runtime_error`, kept in an
  `exceptions/` folder next to the code that throws them. The exception is `luau` and `scripting`: a failure a correct
  program can hit returns a result, and exceptions are only for misuse (`docs/luau/error-handling.md`).

### Files

- Named in PascalCase after their class: `Window.hpp`, `Window.cpp`. Headers are `.hpp`, sources `.cpp`, template
  implementations `.tpp`.
- Every source and header file starts with the Epitech header, with the current year, the project name (`rtype`) and the
  file name without extension as the description:

  ```cpp
  /*
  ** EPITECH PROJECT, 2026
  ** rtype
  ** File description:
  ** Window
  */
  ```

- Headers use `#pragma once`, right after the Epitech header. Never `#ifndef` guards.
- Namespaces mirror the directory path under `src/rtype/` (`rtype::contract::platform::input`,
  `rtype::platform::glfw`, `rtype::render::vulkan`). The exceptions are listed in the `AGENTS.md` of the module that has
  them (the ECS feature folders and the themes of the renderer contract).
- Includes: a sibling file (same folder) by its bare name (`"Window.hpp"`); anything else from the `src/` root as
  `"rtype/<module>/..."`. Never a relative path going up the tree (`"../MyInclude.hpp"`), as much as possible really.

### Classes

- Private members are prefixed with `_` (`_window`) and documented with `///<` comments.
- Accessors are Google-style: a getter is `x()`, a setter `setX()`, in camelCase. This applies to new and touched code;
  old `getX()` is not renamed in bulk.
- The rule of five is mandatory: every class explicitly declares its destructor, copy constructor, copy assignment, move
  constructor and move assignment, each `= default` or `= delete` when there is nothing custom to write. Never rely on
  implicit special members. Copy and move are explicitly deleted for resource-owning classes (RAII).
- `[[nodiscard]]` on getters, `noexcept` where applicable.
- Members follow the Google declaration order: `public`, then `protected`, then `private` sections (omit empty ones);
  within each section, in this order: types and type aliases, static constants, factory functions, constructors, the
  destructor and the copy/move special members, all other member functions (static and non-static), then data members.
  Within those groups:
    1. Custom constructors first (parameterized, then default).
    2. The rule of five in textbook order, right after the custom constructors: destructor, copy constructor, copy
       assignment, move constructor, move assignment.
    3. Accessors: getters, then setters, in the same order as the data members they expose.
    4. Other member functions: public behavior in logical groups, static functions together.
    5. Data members, in the same order as their accessors.

### Names and comments

- Constants (`constexpr` and `static constexpr` variables) and enumerators are named `kName` (`kGamepadAxisCount`,
  `Key::kEscape`, `GamepadButton::kSouth`). Enum types keep PascalCase (`GamepadButton`). Local `const` variables keep
  camelCase.
- Doxygen comments use the `///` style (not `/** ... */`), with `@brief` on public classes and functions.

### Templates

Template implementations go in a `.tpp` file next to the `.hpp` that declares them (`Pool.hpp` and `Pool.tpp`), not
inline in the `.hpp`. The `.hpp` only declares the template and ends by including its `.tpp`; the `.tpp` carries the
Epitech header and `#pragma once`, and is never included from anywhere else. A `.tpp` may, and should, `#include` the
standard and project headers its code uses, instead of relying on what its `.hpp` happens to include. Because it must
never be included directly, it has, right after `#pragma once`, a check that fails when its `.hpp` is not the includer:
the `.hpp` defines `RTYPE_<NAME>_INCLUDING_TPP` around the include and the `.tpp` tests it.

```cpp
// Pool.hpp
#define RTYPE_POOL_INCLUDING_TPP
#include "Pool.tpp"  // IWYU pragma: export
#undef RTYPE_POOL_INCLUDING_TPP

// Pool.tpp (after the Epitech header)
#pragma once

#ifndef RTYPE_POOL_INCLUDING_TPP
#error "Pool.tpp is an implementation detail of Pool.hpp: include Pool.hpp instead"
#endif

#include <cassert>
#include <utility>
```

### Practices

- Never `using namespace` in `src/` (headers or sources) or in tests: qualify names. It is allowed only in the `.cpp`
  files of `examples/`, to keep them short.
- Prefer the short form over the verbose one:
    - Test a `std::optional` with its `bool` conversion: `if (opt)`, not `opt.has_value()`.
    - Initialize with braces: `MyClass hello{arg1, arg2};` and `: _member{value}`, not `MyClass hello(arg1, arg2);`.
      Braces reject narrowing conversions, so cast explicitly when one is intended. Keep parentheses where braces would
      call a different constructor (`std::initializer_list` overloads: `std::vector<int> v(3, 0);` builds `{0, 0, 0}`
      while `v{3, 0}` builds `{3, 0}`).
    - Build strings with `std::format`: `std::format("Unknown setting '{}' in '{}'", key, context)`, not concatenation
      with `+`. It takes `std::string_view` and numbers directly (no `std::string(...)`, no `std::to_string`). Use a raw
      string literal (`R"(...)"`) when the text contains quotes. Appending in a loop (`result += ...`) stays as it is.
- Manage resources with RAII and smart pointers: `std::unique_ptr` by default, `std::shared_ptr` only for genuinely
  shared ownership. No raw owning pointers, and no `new`/`delete` or `malloc`/`free` outside code that wraps a C API.

### Exports

Exported symbols of a shared library use that library's own macro, named after the target: `RTYPE_ECS_API`,
`RTYPE_NETWORK_API`, `RTYPE_REPLICATION_API`, `RTYPE_ENGINE_API`, `RTYPE_SCRIPTING_API`,
`RTYPE_CONTRACT_<NAME>_API`, `RTYPE_PLUGIN_<NAME>_API`, `RTYPE_RENDER_VULKAN_API`, and so on
(`__declspec(dllexport)` on Windows). A type with no code of its own (an interface, an exception) takes
`RTYPE_HEADER_ONLY_API` from `rtype/core/Export.hpp` instead. `core` is static and exports nothing.

## Portability and dependencies

- Keep the code portable across Linux, macOS and Windows.
- The project must compile with Clang, Apple Clang and MSVC, and should compile with GCC. Avoid compiler-specific
  extensions and C++23 features that one of the required compilers does not support yet; when a workaround is
  unavoidable, guard it with the appropriate preprocessor check and keep the other compilers working.
- New dependencies go through `add_requires` in the relevant target's `xmake.lua`. A package used by several modules
  (glm, glfw) is declared once in the root `xmake.lua`.
