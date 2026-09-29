/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Error
*/

#include "Error.hpp"

#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#include "ErrorKind.hpp"

namespace rtype::luau {
Error::Error(const ErrorKind kind, std::string message, std::string traceback,
             const std::source_location location) noexcept
    : _kind{kind}, _message{std::move(message)}, _traceback{std::move(traceback)}, _location{location} {}

Error::Error(const ErrorKind kind, std::string message, std::string traceback) noexcept
    : Error{kind, std::move(message), std::move(traceback), std::source_location::current()} {}

Error::Error(const ErrorKind kind, std::string message, const std::source_location location) noexcept
    : Error{kind, std::move(message), "", location} {}

Error::Error(const ErrorKind kind, std::string message) noexcept : Error{kind, std::move(message), ""} {}

ErrorKind Error::kind() const noexcept { return _kind; }

std::string_view Error::message() const noexcept { return _message; }

const char* Error::what() const noexcept { return _message.c_str(); }
}  // namespace rtype::luau
