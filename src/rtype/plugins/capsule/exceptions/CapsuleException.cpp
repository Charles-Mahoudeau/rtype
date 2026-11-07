/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleException
*/

#include "CapsuleException.hpp"

#include <stdexcept>
#include <string>

namespace rtype::plugins::capsule::exceptions {
CapsuleException::CapsuleException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
CapsuleException::~CapsuleException() = default;
}  // namespace rtype::plugins::capsule::exceptions
