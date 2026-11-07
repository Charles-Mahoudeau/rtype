/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPath
*/

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting {
/// @brief A require string, parsed by the rules of the Luau require-by-string proposal. Pure text: it touches no file.
///
/// @details The forms are `./a/b`, `../a/b` (`../` may repeat, only at the start) and `@alias/a/b` or `@alias`. The
/// separator is `/`; the last segment has no extension. See `docs/scripting/plan.md` (D10).
struct RTYPE_SCRIPTING_API ScriptPath {
    /// @brief How the path starts.
    enum class Kind : unsigned char {
        kRelative,  ///< `./` or `../`: from the folder of the requiring file.
        kAlias,     ///< `@name`: from the folder an alias names.
    };

    Kind kind = Kind::kRelative;        ///< The prefix.
    std::size_t parents = 0;            ///< Number of leading `../` (relative paths only).
    std::string alias;                  ///< Alias name, lowercase, without the `@` (alias paths only).
    std::vector<std::string> segments;  ///< Names after the prefix: never empty for a relative path.

    /// @brief Parses a require string.
    /// @return The path, or a `kInvalidPath` error whose message is the reason (no prefix, backslash, empty or
    /// `.`/`..` segment, trailing `/`, extension, empty string).
    [[nodiscard]] static Result<ScriptPath> parse(std::string_view text);

    /// @return Whether @p name is a valid alias name: one or more of `[a-zA-Z0-9_.-]`, and not `.` or `..`.
    [[nodiscard]] static bool isValidAliasName(std::string_view name) noexcept;
};
}  // namespace rtype::scripting
