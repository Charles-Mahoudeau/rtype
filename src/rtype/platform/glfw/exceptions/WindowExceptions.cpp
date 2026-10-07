/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowExceptions
*/

#include "WindowExceptions.hpp"

#include <GLFW/glfw3.h>

#include <format>
#include <stdexcept>
#include <string>

namespace rtype::platform::glfw::exceptions {

GLFWWindowException::GLFWWindowException(const std::string& message) : std::runtime_error(withGlfwError(message)) {}

std::string GLFWWindowException::withGlfwError(const std::string& message) {
    const char* description = nullptr;
    const int errorCode = glfwGetError(&description);
    if (errorCode == GLFW_NO_ERROR) {
        return message;
    }
    if (description != nullptr) {
        return std::format("{}: {}", message, description);
    }
    return std::format("{}: {}", message, errorCode);
}

}  // namespace rtype::platform::glfw::exceptions
