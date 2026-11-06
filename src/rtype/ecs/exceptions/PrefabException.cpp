/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabException
*/

#include "PrefabException.hpp"

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
PrefabException::PrefabException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
PrefabException::~PrefabException() = default;
}  // namespace rtype::ecs::exceptions
