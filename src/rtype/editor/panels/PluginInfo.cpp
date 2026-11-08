/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginInfo
*/

#include "PluginInfo.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/App.hpp"

namespace rtype::editor::panels {

namespace {
/// @return Whether @p text contains @p part, ignoring case; an empty @p part is in everything.
bool containsIgnoringCase(std::string_view text, std::string_view part) {
    const auto found = std::ranges::search(text, part, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
    return !found.empty() || part.empty();
}
}  // namespace

PluginList collectPlugins(const ecs::App& app, std::string_view filter) {
    PluginList list;
    const std::vector<const ecs::Plugin*> plugins = app.getPlugins();
    for (const ecs::Plugin* plugin : plugins) {
        if (!containsIgnoringCase(plugin->getName(), filter)) {
            continue;
        }
        PluginRow row;
        row.name = std::string{plugin->getName()};
        row.isEngine = row.name.starts_with("rtype.");
        for (const std::string_view dependency : plugin->getDependencies()) {
            row.requires_.emplace_back(dependency);
        }
        for (const ecs::Plugin* other : plugins) {
            const auto dependencies = other->getDependencies();
            if (std::ranges::find(dependencies, plugin->getName()) != dependencies.end()) {
                row.requiredBy.emplace_back(other->getName());
            }
        }
        list.loaded.push_back(std::move(row));
    }
    for (std::string& name : app.getPluginRegistry().getNames()) {
        if (!app.hasPlugin(name) && containsIgnoringCase(name, filter)) {
            list.notLoaded.push_back(std::move(name));
        }
    }
    return list;
}

}  // namespace rtype::editor::panels
