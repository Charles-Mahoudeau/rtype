/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendExceptions
*/

#pragma once
#include <stdexcept>
#include <string>

namespace rtype::plugins::backend::exceptions {
/// @brief Exception thrown when the configuration names a platform or a renderer that was not registered.
class BackendException : public std::runtime_error {
  public:
    explicit BackendException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace rtype::plugins::backend::exceptions
