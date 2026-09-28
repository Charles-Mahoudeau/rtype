/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowExceptions
*/

#ifndef WINDOWEXCEPTIONS_HPP_
#define WINDOWEXCEPTIONS_HPP_

#include <stdexcept>
#include <string>

namespace rtype::vulkan::platform::exceptions {
/**
 * @brief Exception thrown when a GLFW window operation fails.
 *
 * This exception is thrown when an error occurs during the creation or management
 * of a GLFW window. It provides a message describing the error.
 */
class GLFWWindowException : public std::runtime_error {
  public:
    explicit GLFWWindowException(const std::string &message) : std::runtime_error(message) {}
};
}  // namespace rtype::vulkan::platform::exceptions
#endif /* !WINDOWEXCEPTIONS_HPP_ */
