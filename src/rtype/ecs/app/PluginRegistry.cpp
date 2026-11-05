/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginRegistry
*/

#include "PluginRegistry.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
std::vector<std::string> PluginRegistry::getNames() const {
    std::vector<std::string> names;
    names.reserve(_factories.size());
    for (const auto& [name, factory] : _factories) {
        names.push_back(name);  // the map keeps them sorted
    }
    return names;
}

void PluginRegistry::add(std::string name, Factory factory) {
    assert(factory != nullptr);
    _factories.insert_or_assign(std::move(name), std::move(factory));
}

bool PluginRegistry::has(std::string_view name) const noexcept { return _factories.contains(name); }

std::unique_ptr<Plugin> PluginRegistry::create(std::string_view name) const {
    const auto found = _factories.find(name);
    if (found == _factories.end()) {
        return nullptr;
    }
    std::unique_ptr<Plugin> plugin = found->second();
    assert(plugin != nullptr);
    return plugin;
}
}  // namespace rtype::ecs
