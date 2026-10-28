/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConfigExceptions
*/

#pragma once
#include <stdexcept>
#include <string>

namespace rtype::core::config::exceptions {
/// @brief Exception thrown when the configuration is wrong: a missing or unknown key, a value of the wrong type.
///
/// This exception is thrown for example when a config file sets `window.title = 42`, or a setting a backend does
/// not know.
class SettingsException : public std::runtime_error {
  public:
    explicit SettingsException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace rtype::core::config::exceptions
