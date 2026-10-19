/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryException
*/

#include "QueryException.hpp"

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
QueryException::QueryException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
QueryException::~QueryException() = default;
}  // namespace rtype::ecs::exceptions
