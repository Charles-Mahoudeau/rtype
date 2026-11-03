/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptLoadException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/scripting/Export.hpp"

namespace rtype::scripting::exceptions {
/// @brief Exception thrown when the scripts of a game cannot be loaded: the scripts directory cannot be read, a file
/// cannot be read, or a script does not compile or fails when it runs.
///
/// @details Plugin::build() cannot return a Result, so ScriptingPlugin reports its failures this way. It can be caught
/// as a `std::runtime_error`.
class RTYPE_SCRIPTING_API ScriptLoadException : public std::runtime_error {
  public:
    explicit ScriptLoadException(const std::string& message);
    ~ScriptLoadException() override;
    ScriptLoadException(const ScriptLoadException& other) = default;
    ScriptLoadException& operator=(const ScriptLoadException& other) = default;
    ScriptLoadException(ScriptLoadException&& other) noexcept = default;
    ScriptLoadException& operator=(ScriptLoadException&& other) noexcept = default;
};
}  // namespace rtype::scripting::exceptions
