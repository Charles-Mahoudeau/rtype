/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendRegistry
*/

#include "BackendRegistry.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/config/Settings.hpp"
#include "engine/exceptions/ConfigExceptions.hpp"
#include "engine/platform/WindowConfig.hpp"

namespace rtype::engine::backend {

namespace {

/// @return The names of a factory map, in order.
template <typename Factory>
std::vector<std::string> namesOf(const std::map<std::string, Factory, std::less<>>& factories) {
    std::vector<std::string> names;
    names.reserve(factories.size());
    for (const auto& [name, factory] : factories) {
        names.push_back(name);
    }
    return names;
}

/// @return The factory registered under the name the config gives for `kind` ("platform" or "renderer").
/// @throws exceptions::BackendException If the config has no such name, or nothing is registered under it.
template <typename Factory>
const Factory& findFactory(const std::map<std::string, Factory, std::less<>>& factories, const config::Settings& config,
                           std::string_view kind, std::string& name) {
    name = config.getString(kind, "");
    if (name.empty()) {
        throw exceptions::BackendException("The config does not choose a " + std::string(kind) + " ('" +
                                           std::string(kind) + " = \"...\"')");
    }
    const auto it = factories.find(name);
    if (it == factories.end()) {
        std::string available;
        for (const std::string& known : namesOf(factories)) {
            available += (available.empty() ? "" : ", ") + known;
        }
        throw exceptions::BackendException("Unknown " + std::string(kind) + " '" + name +
                                           "' (registered: " + (available.empty() ? "none" : available) + ")");
    }
    return it->second;
}

}  // namespace

void BackendRegistry::addPlatform(std::string name, PlatformFactory factory) {
    _platforms.insert_or_assign(std::move(name), std::move(factory));
}

void BackendRegistry::addRenderer(std::string name, RendererFactory factory) {
    _renderers.insert_or_assign(std::move(name), std::move(factory));
}

std::vector<std::string> BackendRegistry::getPlatformNames() const { return namesOf(_platforms); }

std::vector<std::string> BackendRegistry::getRendererNames() const { return namesOf(_renderers); }

Backend BackendRegistry::createBackend(const config::Settings& config) const {
    std::string platformName;
    std::string rendererName;
    const PlatformFactory& createPlatform = findFactory(_platforms, config, "platform", platformName);
    const RendererFactory& createRenderer = findFactory(_renderers, config, "renderer", rendererName);

    const platform::WindowConfig window = platform::WindowConfig::fromSettings(config.section("window"));

    Backend backend;
    backend.platform = createPlatform(config.section(platformName));
    if (!backend.platform) {
        throw exceptions::BackendException("The platform factory '" + platformName + "' returned no platform");
    }
    backend.renderer = createRenderer(config.section(rendererName));
    if (!backend.renderer) {
        throw exceptions::BackendException("The renderer factory '" + rendererName + "' returned no renderer");
    }
    backend.renderer->prepare(*backend.platform);
    backend.platform->init(window);
    backend.renderer->init(*backend.platform);
    return backend;
}

}  // namespace rtype::engine::backend
