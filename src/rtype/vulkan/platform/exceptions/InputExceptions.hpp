/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputExceptions
*/

#pragma once
#include <stdexcept>
#include <string>

namespace rtype::vulkan::platform::exceptions {
/**
 * @brief Exception thrown when an input action is used incorrectly.
 *
 * This exception is thrown for example when an action is requested by a name
 * that was never registered with Input::addAction().
 */
class InputException : public std::runtime_error {
  public:
    explicit InputException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace rtype::vulkan::platform::exceptions
