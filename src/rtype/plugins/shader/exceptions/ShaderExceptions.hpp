/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::plugins::shader::exceptions {
/// @brief Exception thrown when a shader cannot be compiled: a declaration the engine does not understand, a value
/// past its budget, or GLSL the compiler rejects. Its message names the shader and the line.
class RTYPE_HEADER_ONLY_API ShaderException : public std::runtime_error {
  public:
    explicit ShaderException(const std::string& message) : std::runtime_error{message} {}
    ~ShaderException() override = default;
    ShaderException(const ShaderException& other) = default;
    ShaderException& operator=(const ShaderException& other) = default;
    ShaderException(ShaderException&& other) noexcept = default;
    ShaderException& operator=(ShaderException&& other) noexcept = default;
};
}  // namespace rtype::plugins::shader::exceptions
