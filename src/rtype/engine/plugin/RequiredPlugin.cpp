/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RequiredPlugin
*/

#include "RequiredPlugin.hpp"

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/engine/Exceptions.hpp"

namespace rtype::engine::plugin {
std::unique_ptr<ecs::Plugin> createRequiredPlugin(const ecs::PluginRegistry& registry, const std::string_view name,
                                                  const std::vector<std::filesystem::path>& searched) {
    std::unique_ptr<ecs::Plugin> plugin = registry.create(name);
    if (plugin != nullptr) {
        return plugin;
    }
    std::string folders;
    for (const std::filesystem::path& folder : searched) {
        folders += std::format("{}'{}'", folders.empty() ? "" : ", ", folder.generic_string());
    }
    std::string known;
    for (const std::string& registered : registry.getNames()) {
        known += std::format("{}'{}'", known.empty() ? "" : ", ", registered);
    }
    throw exceptions::RuntimeError{std::format(
        "The plugin '{}', which every game needs, is provided by no library (looked in {}; known plugins: {})", name,
        folders.empty() ? "no folder" : folders, known.empty() ? "none" : known)};
}
}  // namespace rtype::engine::plugin
