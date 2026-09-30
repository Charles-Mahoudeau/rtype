/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowExceptions
*/

#pragma once
#include <stdexcept>
#include <string>

namespace rtype::platform::exceptions {
/// @brief Exception thrown when a GLFW window operation fails.
///
/// This exception is thrown when an error occurs during the creation or management
/// of a GLFW window. Its message is the given description, followed by the pending
/// GLFW error (read with `glfwGetError()`) when there is one.
class GLFWWindowException : public std::runtime_error {
  public:
    /// @param message Description of the failed operation.
    /// @note Consumes the pending GLFW error: throw it right after the failing GLFW call.
    explicit GLFWWindowException(const std::string& message);
    ~GLFWWindowException() override = default;
    GLFWWindowException(const GLFWWindowException&) = default;
    GLFWWindowException& operator=(const GLFWWindowException&) = default;
    GLFWWindowException(GLFWWindowException&&) = default;
    GLFWWindowException& operator=(GLFWWindowException&&) = default;

  private:
    /// @return The message, followed by the pending GLFW error description or code if any.
    static std::string withGlfwError(const std::string& message);
};
}  // namespace rtype::platform::exceptions
