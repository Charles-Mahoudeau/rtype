/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderExceptions
*/

#pragma once

#include "plugins/backend/exceptions/ShaderExceptions.hpp"

namespace rtype::plugins::shader::exceptions {
/// @brief The contract's ShaderException, which the drawing plugins share: thrown here when a shader cannot be
/// compiled or a value does not match it.
using ShaderException = backend::exceptions::ShaderException;
}  // namespace rtype::plugins::shader::exceptions
