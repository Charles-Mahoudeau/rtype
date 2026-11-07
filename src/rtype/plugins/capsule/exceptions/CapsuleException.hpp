/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "plugins/capsule/Export.hpp"

namespace rtype::plugins::capsule::exceptions {
/// @brief Exception thrown when a capsule is misused: an unknown capsule, path or component, a value of the wrong size,
/// a node without a parent, or capsules that would contain each other. The message names the capsule involved.
/// @details Exported, with its destructor defined in the library: it is thrown inside the library and caught in the
/// game, so its type information must be one, shared object (macOS compares exception types by address).
class RTYPE_PLUGIN_CAPSULE_API CapsuleException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the capsule involved.
    explicit CapsuleException(const std::string& message);
    ~CapsuleException() override;
    CapsuleException(const CapsuleException& other) = default;
    CapsuleException& operator=(const CapsuleException& other) = default;
    CapsuleException(CapsuleException&& other) noexcept = default;
    CapsuleException& operator=(CapsuleException&& other) noexcept = default;
};
}  // namespace rtype::plugins::capsule::exceptions
