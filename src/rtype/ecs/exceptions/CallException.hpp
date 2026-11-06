/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when an engine call cannot be registered (its name is taken) or used (an unknown id, or
/// arguments or a result of the wrong size). The message names the call.
/// @details Exported, with its destructor defined in the library: it is thrown inside the library and caught in the
/// game, so its type information must be one, shared object (macOS compares exception types by address).
class RTYPE_ECS_API CallException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the call.
    explicit CallException(const std::string& message);
    ~CallException() override;
    CallException(const CallException& other) = default;
    CallException& operator=(const CallException& other) = default;
    CallException(CallException&& other) noexcept = default;
    CallException& operator=(CallException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
