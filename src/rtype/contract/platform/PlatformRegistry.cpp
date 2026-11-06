/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlatformRegistry
*/

#include "rtype/contract/platform/PlatformRegistry.hpp"

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "rtype/contract/platform/exceptions/BackendExceptions.hpp"

namespace rtype::contract::platform {

std::vector<std::string> PlatformRegistry::getNames() const {
    std::vector<std::string> names;
    names.reserve(_factories.size());
    for (const auto& [name, factory] : _factories) {
        names.push_back(name);
    }
    return names;
}

void PlatformRegistry::add(std::string name, Factory factory) {
    _factories.insert_or_assign(std::move(name), std::move(factory));
}

std::unique_ptr<IPlatform> PlatformRegistry::create(const core::config::Settings& config) const {
    const std::string name = config.getString("platform", "");
    if (name.empty()) {
        throw exceptions::BackendException{R"(The config does not choose a platform ('platform = "..."'))"};
    }
    const auto found = _factories.find(name);
    if (found == _factories.end()) {
        std::string available;
        for (const std::string& known : getNames()) {
            available += (available.empty() ? "" : ", ") + known;
        }
        throw exceptions::BackendException{
            std::format("Unknown platform '{}' (registered: {})", name, available.empty() ? "none" : available)};
    }
    std::unique_ptr<IPlatform> created = found->second(config.section(name));
    if (!created) {
        throw exceptions::BackendException{std::format("The platform factory '{}' returned no platform", name)};
    }
    return created;
}

}  // namespace rtype::contract::platform
