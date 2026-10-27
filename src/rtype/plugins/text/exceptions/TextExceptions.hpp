/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::plugins::text::exceptions {
/// @brief Exception thrown when a font cannot be loaded: its metrics file cannot be read or parsed, or it is not an
/// MSDF atlas.
class FontException : public std::runtime_error {
  public:
    explicit FontException(const std::string& message) : std::runtime_error{message} {}
    ~FontException() override = default;
    FontException(const FontException& other) = default;
    FontException& operator=(const FontException& other) = default;
    FontException(FontException&& other) noexcept = default;
    FontException& operator=(FontException&& other) noexcept = default;
};
}  // namespace rtype::plugins::text::exceptions
