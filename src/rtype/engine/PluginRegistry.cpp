/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginRegistry
*/

#include "PluginRegistry.hpp"

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Exceptions.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::engine {
std::vector<std::string> PluginRegistry::getNames() const {
    std::vector<std::string> names;
    names.reserve(_factories.size());
    for (const auto& [name, factory] : _factories) {
        names.push_back(name);  // the map keeps them sorted
    }
    return names;
}

void PluginRegistry::add(std::string name, Factory factory) { _factories[std::move(name)] = std::move(factory); }

bool PluginRegistry::has(std::string_view name) const { return _factories.contains(name); }

std::unique_ptr<ecs::Plugin> PluginRegistry::create(std::string_view name) const {
    const auto found = _factories.find(name);
    if (found == _factories.end()) {
        std::string known;
        for (const std::string& registered : getNames()) {
            known += std::format("{}'{}'", known.empty() ? "" : ", ", registered);
        }
        throw exceptions::RuntimeError{std::format("Unknown plugin '{}' in the project settings; known plugins: {}",
                                                   name, known.empty() ? "none" : known)};
    }
    return found->second();
}
}  // namespace rtype::engine
