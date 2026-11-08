/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsolePrelude
*/

#pragma once

#include <string_view>

namespace rtype::scripting::console {
/// @brief Name of the prelude in error messages.
inline constexpr std::string_view kConsolePreludeName = "console.prelude";

/// @return The Luau source of the debug console's helpers: it returns a table with `format(...)` (writes the values
/// of a result), `complete(roots, word)` (the completions of a dotted name) and `engineView(engine)` (`engine` without
/// the declarations, which a console must not make while the schedule runs).
[[nodiscard]] std::string_view getConsolePrelude() noexcept;
}  // namespace rtype::scripting::console
