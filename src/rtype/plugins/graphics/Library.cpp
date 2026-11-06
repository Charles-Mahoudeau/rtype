/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>
#include <utility>

#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureCalls.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::graphics::addTextureForms(registrar.getPlugins());
    using rtype::plugins::graphics::BackendRegistry;
    using rtype::plugins::graphics::GraphicsPlugin;
    // Filled by the backend libraries, then given to the plugin when it is built.
    auto backends = std::make_shared<std::unique_ptr<BackendRegistry>>(std::make_unique<BackendRegistry>());
    registrar.getLibrary().provide(rtype::contract::platform::kPlatformRegistryService, (*backends)->platforms);
    registrar.getLibrary().provide(rtype::contract::renderer::kRendererRegistryService, (*backends)->renderers);
    registrar.getPlugins().add(std::string{GraphicsPlugin::kName},
                               [backends](const rtype::core::config::Settings& settings) {
                                   return std::make_unique<GraphicsPlugin>(std::move(*backends), settings);
                               });
}
