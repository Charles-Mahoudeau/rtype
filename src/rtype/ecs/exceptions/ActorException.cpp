/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorException
*/

#include "ActorException.hpp"

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
ActorException::ActorException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
ActorException::~ActorException() = default;
}  // namespace rtype::ecs::exceptions
