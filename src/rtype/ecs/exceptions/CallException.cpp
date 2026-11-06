/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallException
*/

#include "CallException.hpp"

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
CallException::CallException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
CallException::~CallException() = default;
}  // namespace rtype::ecs::exceptions
