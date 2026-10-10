/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ErrorKind
*/

#pragma once

#include <cstdint>

namespace rtype::luau {
/// @brief Categories of failure that Luau creation, compilation or execution can report.
enum class ErrorKind : std::uint8_t {
    kRuntime,        ///< The script raised an error while running.
    kSyntax,         ///< The script could not be compiled because of a syntax error.
    kTypeMismatch,   ///< A value had a different type than the one expected.
    kTimeout,        ///< Execution exceeded its allotted time.
    kOutOfMemory,    ///< The Luau state could not allocate memory.
    kStackOverflow,  ///< The Luau stack or call depth limit was exceeded.
    kCompilation,    ///< The script could not be compiled because of a compilation error.
    kUnknown,        ///< The failure does not fit any other category.
};
}  // namespace rtype::luau
