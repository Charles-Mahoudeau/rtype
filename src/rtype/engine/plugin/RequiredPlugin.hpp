/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RequiredPlugin
*/

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/engine/Export.hpp"

namespace rtype::engine::plugin {
/// @brief Builds a plugin every game needs from what the loaded libraries registered, as the plugins a game lists are:
/// the Engine links none of them.
/// @param registry The plugins the libraries registered.
/// @param name The plugin's name, such as "rtype.Transform".
/// @param searched The folders the libraries were looked for in, named in the error.
/// @return The plugin, never null.
/// @throws exceptions::RuntimeError When no library registered @p name: the message names it, the folders searched
/// and the plugins that exist.
[[nodiscard]] RTYPE_ENGINE_API std::unique_ptr<ecs::Plugin> createRequiredPlugin(
    const ecs::PluginRegistry& registry, std::string_view name, const std::vector<std::filesystem::path>& searched);
}  // namespace rtype::engine::plugin
