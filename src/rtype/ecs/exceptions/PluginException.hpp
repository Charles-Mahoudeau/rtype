/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a plugin's dependencies cannot be resolved: a required plugin that was neither added
/// nor registered, or plugins requiring each other in a cycle. The message names the plugins involved.
/// @details Exported, with its destructor defined in the library: it is thrown inside the library and caught in the
/// game, so its type information must be one, shared object (macOS compares exception types by address).
class RTYPE_ECS_API PluginException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the plugins involved.
    explicit PluginException(const std::string& message);
    ~PluginException() override;
    PluginException(const PluginException& other) = default;
    PluginException& operator=(const PluginException& other) = default;
    PluginException(PluginException&& other) noexcept = default;
    PluginException& operator=(PluginException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
