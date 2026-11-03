/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Error
*/

#include "Error.hpp"

#include <string>
#include <string_view>
#include <utility>

#include "ErrorKind.hpp"

namespace rtype::scripting {
Error::Error(const ErrorKind kind, std::string message) : _kind{kind}, _message{std::move(message)} {}

ErrorKind Error::kind() const noexcept { return _kind; }

std::string_view Error::message() const noexcept { return _message; }
}  // namespace rtype::scripting
