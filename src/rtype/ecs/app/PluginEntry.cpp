/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginEntry
*/

#include "PluginEntry.hpp"

#include <format>
#include <string_view>

#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"

namespace rtype::ecs {
PluginRegistrar::PluginRegistrar(PluginRegistry& plugins) noexcept : _plugins{&plugins} {}

PluginRegistry& PluginRegistrar::getPlugins() noexcept { return *_plugins; }

void checkPluginLibrary(const PluginLibrary& library, const std::string_view file) {
    if (library.abiVersion != kPluginAbiVersion) {
        throw exceptions::PluginException{
            std::format("Plugin library '{}' was built for the plugin version {}, the engine has {}: rebuild it", file,
                        library.abiVersion, kPluginAbiVersion)};
    }
    const std::string_view toolchain = library.toolchain != nullptr ? library.toolchain : "";
    if (toolchain != RTYPE_ECS_PLUGIN_TOOLCHAIN) {
        throw exceptions::PluginException{
            std::format("Plugin library '{}' was built with '{}', the engine with '{}': rebuild it", file, toolchain,
                        RTYPE_ECS_PLUGIN_TOOLCHAIN)};
    }
}
}  // namespace rtype::ecs
