/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RendererRegistry
*/

#include "rtype/contract/renderer/RendererRegistry.hpp"

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "rtype/contract/platform/exceptions/BackendExceptions.hpp"

namespace rtype::contract::renderer {

std::vector<std::string> RendererRegistry::getNames() const {
    std::vector<std::string> names;
    names.reserve(_factories.size());
    for (const auto& [name, factory] : _factories) {
        names.push_back(name);
    }
    return names;
}

void RendererRegistry::add(std::string name, Factory factory) {
    _factories.insert_or_assign(std::move(name), std::move(factory));
}

std::unique_ptr<IRenderer> RendererRegistry::create(const core::config::Settings& config) const {
    const std::string name = config.getString("renderer", "");
    if (name.empty()) {
        throw platform::exceptions::BackendException{R"(The config does not choose a renderer ('renderer = "..."'))"};
    }
    const auto found = _factories.find(name);
    if (found == _factories.end()) {
        std::string available;
        for (const std::string& known : getNames()) {
            available += (available.empty() ? "" : ", ") + known;
        }
        throw platform::exceptions::BackendException{
            std::format("Unknown renderer '{}' (registered: {})", name, available.empty() ? "none" : available)};
    }
    std::unique_ptr<IRenderer> created = found->second(config.section(name));
    if (!created) {
        throw platform::exceptions::BackendException{
            std::format("The renderer factory '{}' returned no renderer", name)};
    }
    return created;
}

}  // namespace rtype::contract::renderer
