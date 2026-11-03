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
};
}  // namespace rtype::scripting
