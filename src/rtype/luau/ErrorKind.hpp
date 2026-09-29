/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ErrorKind
*/

#pragma once

#include <cstdint>

namespace rtype::luau {
enum class ErrorKind : std::uint8_t {
    kRuntime,
    kSyntax,
    kTypeMismatch,
    kTimeout,
    kOutOfMemory,
    kStackOverflow,
    kUnknown
};
}
