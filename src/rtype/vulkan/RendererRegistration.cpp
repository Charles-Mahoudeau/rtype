/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RendererRegistration
*/

#include "RendererRegistration.hpp"

#include <memory>
#include <string>
#include <vulkan/vulkan_raii.hpp>

#include "VulkanRenderer.hpp"
#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/exceptions/ConfigExceptions.hpp"
#include "engine/graphics/IRenderer.hpp"

namespace rtype::vulkan {

namespace {

/// @return The severity named in the config.
/// @throws engine::exceptions::SettingsException If the name is unknown.
vk::DebugUtilsMessageSeverityFlagBitsEXT toSeverity(const std::string& name) {
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    if (name == "verbose") {
        return Severity::eVerbose;
    }
    if (name == "info") {
        return Severity::eInfo;
    }
    if (name == "warning") {
        return Severity::eWarning;
    }
    if (name == "error") {
        return Severity::eError;
    }
    throw engine::exceptions::SettingsException(
        "Setting 'vulkan.minSeverity' must be \"verbose\", \"info\", "
        "\"warning\" or \"error\", not \"" +
        name + "\"");
}

/// @return The Config described by the "vulkan" section; missing keys keep the Config defaults.
VulkanRenderer::Config toConfig(const engine::config::Settings& settings) {
    settings.checkKeys({"engineName", "layers", "extraExtensions", "debugging", "minSeverity"}, "vulkan");
    VulkanRenderer::Config config;
    config.engineName = settings.getString("engineName", config.engineName);
    config.layers = settings.getStringList("layers", config.layers);
    config.extraExtensions = settings.getStringList("extraExtensions", config.extraExtensions);
    config.debugging = settings.getBool("debugging", config.debugging);
    if (settings.has("minSeverity")) {
        config.minSeverity = toSeverity(settings.getString("minSeverity", ""));
    }
    return config;
}

}  // namespace

void registerRenderers(engine::backend::BackendRegistry& registry) {
    registry.addRenderer("vulkan",
                         [](const engine::config::Settings& settings) -> std::unique_ptr<engine::graphics::IRenderer> {
                             return std::make_unique<VulkanRenderer>(toConfig(settings));
                         });
}

}  // namespace rtype::vulkan
