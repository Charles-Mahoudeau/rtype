/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when an actor is misused: an unknown or duplicate name, an unknown component, a value of the
/// wrong size, or child actors that would contain each other. The message names the actor involved.
/// @details Exported, with its destructor defined in the library: it is thrown inside the library and caught in the
/// game, so its type information must be one, shared object (macOS compares exception types by address).
class RTYPE_ECS_API ActorException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the actor involved.
    explicit ActorException(const std::string& message);
    ~ActorException() override;
    ActorException(const ActorException& other) = default;
    ActorException& operator=(const ActorException& other) = default;
    ActorException(ActorException&& other) noexcept = default;
    ActorException& operator=(ActorException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
