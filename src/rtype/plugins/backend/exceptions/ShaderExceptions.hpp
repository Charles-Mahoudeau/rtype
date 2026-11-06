/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::plugins::backend::exceptions {
/// @brief Exception thrown when a shader cannot be compiled (a declaration the engine does not understand, a value past
/// its budget, GLSL the compiler rejects), or when a value set by name does not match the shader (an unknown
/// `uniform`, the wrong type). Its message names the shader, and the line or the value.
class ShaderException : public std::runtime_error {
  public:
    explicit ShaderException(const std::string& message) : std::runtime_error{message} {}
    ~ShaderException() override = default;
    ShaderException(const ShaderException& other) = default;
    ShaderException& operator=(const ShaderException& other) = default;
    ShaderException(ShaderException&& other) noexcept = default;
    ShaderException& operator=(ShaderException&& other) noexcept = default;
};
}  // namespace rtype::plugins::backend::exceptions
