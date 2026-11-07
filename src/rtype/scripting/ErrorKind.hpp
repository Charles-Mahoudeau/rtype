/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ErrorKind
*/

#pragma once

#include <cstdint>

namespace rtype::scripting {
/// @brief Categories of failure the scripting bridge reports through Result.
enum class ErrorKind : std::uint8_t {
    kInvalidDeclaration,  ///< A declaration is inconsistent: empty or duplicate name, bad default value.
    kLayoutMismatch,      ///< A component name is already registered with another layout.
    kRegistration,        ///< The ECS refused the component for another reason.
    kInvalidValue,        ///< A script value does not fit the field it is written to, or the field cannot be read.
    kInvalidPath,         ///< A require path or a script entry is malformed (no prefix, backslash, extension...).
    kModuleNotFound,      ///< No file matches a require path, or the file cannot be read.
    kAmbiguousModule,     ///< Both `<path>.luau` and `<path>/init.luau` exist.
    kOutsideScripts,      ///< The path leaves `scripts/`, by `..` or through a link.
    kCaseMismatch,        ///< A segment differs from the name on disk only by its case.
    kUnknownAlias,        ///< `@name` is not a known alias.
};
}  // namespace rtype::scripting
