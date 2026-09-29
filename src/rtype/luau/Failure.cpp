/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Failure
*/

#include "Failure.hpp"

#include <source_location>
#include <string>
#include <utility>

#include "Error.hpp"
#include "ErrorKind.hpp"

namespace rtype::luau {
Failure::Failure(const ErrorKind kind, std::string message, std::string traceback,
                 const std::source_location location) noexcept
    : _error{kind, std::move(message), std::move(traceback), location} {}

Failure::Failure(const ErrorKind kind, std::string message, const std::source_location location) noexcept
    : _error{kind, std::move(message), location} {}
}  // namespace rtype::luau
