/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginException
*/

#include "PluginException.hpp"

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
PluginException::PluginException(const std::string& message) : std::runtime_error{message} {}

// Defined here, not in the header: the library is the one place that emits the class's type information.
PluginException::~PluginException() = default;
}  // namespace rtype::ecs::exceptions
