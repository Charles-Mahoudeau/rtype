/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptLoadException
*/

#include "rtype/scripting/exceptions/ScriptLoadException.hpp"

#include <string>

namespace rtype::scripting::exceptions {
ScriptLoadException::ScriptLoadException(const std::string& message) : std::runtime_error{message} {}

ScriptLoadException::~ScriptLoadException() = default;
}  // namespace rtype::scripting::exceptions
