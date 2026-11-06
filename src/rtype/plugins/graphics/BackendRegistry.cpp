/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendRegistry
*/

#include "plugins/graphics/BackendRegistry.hpp"

#include "core/config/Settings.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"

namespace rtype::plugins::graphics {

Backend BackendRegistry::createBackend(const core::config::Settings& config) const {
    const contract::platform::WindowConfig window =
        contract::platform::WindowConfig::fromSettings(config.section("window"));
    Backend backend;
    backend.platform = platforms.create(config);
    backend.renderer = renderers.create(config);
    backend.renderer->setup(*backend.platform);
    backend.platform->init(window);
    backend.renderer->init(*backend.platform);
    return backend;
}

}  // namespace rtype::plugins::graphics
