/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::plugins::json::exceptions {
/// @brief Exception thrown when a value cannot be encoded (a NaN or infinite number, a malformed node stream from
/// JsonForms.luau) or a text cannot be decoded (malformed JSON, nesting over 64 levels, a number no finite double can
/// hold). The engine call mechanism (rtype::scripting::call::callEngine) turns it into a Luau error naming the call.
class RTYPE_HEADER_ONLY_API JsonException : public std::runtime_error {
  public:
    explicit JsonException(const std::string& message) : std::runtime_error{message} {}
    ~JsonException() override = default;
    JsonException(const JsonException& other) = default;
    JsonException& operator=(const JsonException& other) = default;
    JsonException(JsonException&& other) noexcept = default;
    JsonException& operator=(JsonException&& other) noexcept = default;
};
}  // namespace rtype::plugins::json::exceptions
