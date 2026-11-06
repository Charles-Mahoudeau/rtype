/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::plugins::input::exceptions {
/// @brief Exception thrown when an input action is used incorrectly.
///
/// This exception is thrown for example when an action is requested by a name
/// that was never registered with Input::addAction().
class RTYPE_HEADER_ONLY_API InputException : public std::runtime_error {
  public:
    explicit InputException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace rtype::plugins::input::exceptions
