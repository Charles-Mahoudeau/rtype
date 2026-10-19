/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "Registration.hpp"

#include <array>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "VulkanRenderer.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/exceptions/ConfigExceptions.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"

namespace rtype::render::vulkan {

namespace {

/// @return The severity named in the config.
/// @throws plugins::backend::exceptions::SettingsException If the name is unknown.
vk::DebugUtilsMessageSeverityFlagBitsEXT toSeverity(std::string_view name) {
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using Entry = std::pair<std::string_view, Severity>;
    static constexpr std::array<Entry, 4> kSeverities{{
        {"verbose", Severity::eVerbose},
        {"info", Severity::eInfo},
        {"warning", Severity::eWarning},
        {"error", Severity::eError},
    }};

    for (const auto& [key, severity] : kSeverities) {
        if (key == name) {
            return severity;
        }
    }
    throw plugins::backend::exceptions::SettingsException(
        std::format(R"(Setting 'vulkan.minSeverity' must be "verbose", "info", "warning" or "error", not "{}")", name));
}

/// @return The physical device type named in the config.
/// @throws plugins::backend::exceptions::SettingsException If the name is unknown.
vk::PhysicalDeviceType toDeviceType(std::string_view name) {
    using Type = vk::PhysicalDeviceType;
    using Entry = std::pair<std::string_view, Type>;
    static constexpr std::array<Entry, 5> kDeviceTypes{{
        {"discrete_gpu", Type::eDiscreteGpu},
        {"integrated_gpu", Type::eIntegratedGpu},
        {"virtual_gpu", Type::eVirtualGpu},
        {"cpu", Type::eCpu},
        {"other", Type::eOther},
    }};

    for (const auto& [key, type] : kDeviceTypes) {
        if (key == name) {
            return type;
        }
    }
    throw plugins::backend::exceptions::SettingsException(std::format(
        R"(Setting 'vulkan.preferredDeviceType' must be "discrete_gpu", "integrated_gpu", "virtual_gpu", "cpu" or )"
        R"("other", not "{}")",
        name));
}

/// @return The present mode named in the config.
/// @throws plugins::backend::exceptions::SettingsException If the name is unknown.
vk::PresentModeKHR toPresentMode(std::string_view name) {
    using Mode = vk::PresentModeKHR;
    using Entry = std::pair<std::string_view, Mode>;
    static constexpr std::array<Entry, 3> kPresentModes{{
        {"fifo", Mode::eFifo},
        {"mailbox", Mode::eMailbox},
        {"immediate", Mode::eImmediate},
    }};

    for (const auto& [key, mode] : kPresentModes) {
        if (key == name) {
            return mode;
        }
    }
    throw plugins::backend::exceptions::SettingsException(
        std::format(R"(Setting 'vulkan.presentMode' must be "fifo", "mailbox" or "immediate", not "{}")", name));
}

/// @return The Config described by the "vulkan" section; missing keys keep the Config defaults.
VulkanRenderer::Config toConfig(const plugins::backend::config::Settings& settings) {
    settings.checkKeys(
        {"engineName", "layers", "extraExtensions", "debugging", "minSeverity", "preferredDeviceType",
         "synchronizationValidation", "bestPractices", "presentMode", "shaderDirectory", "virtualResolution"},
        "vulkan");
    VulkanRenderer::Config config;
    config.engineName = settings.getString("engineName", config.engineName);
    config.layers = settings.getStringList("layers", config.layers);
    config.extraExtensions = settings.getStringList("extraExtensions", config.extraExtensions);
    config.debugging = settings.getBool("debugging", config.debugging);
    config.synchronizationValidation = settings.getBool("synchronizationValidation", config.synchronizationValidation);
    config.bestPractices = settings.getBool("bestPractices", config.bestPractices);
    if (settings.has("minSeverity")) {
        config.minSeverity = toSeverity(settings.getString("minSeverity", ""));
    }
    if (settings.has("preferredDeviceType")) {
        config.preferredDeviceType = toDeviceType(settings.getString("preferredDeviceType", ""));
    }
    if (settings.has("shaderDirectory")) {
        config.shaderDirectory = settings.getString("shaderDirectory", "");
    }
    if (settings.has("virtualResolution")) {
        const std::vector<double> size = settings.getNumberList("virtualResolution", {});
        if (size.size() != 2 || size.at(0) <= 0.0 || size.at(1) <= 0.0) {
            throw plugins::backend::exceptions::SettingsException(
                "Setting 'vulkan.virtualResolution' must be { width, height }, both positive");
        }
        config.virtualResolution = {static_cast<unsigned>(size.at(0)), static_cast<unsigned>(size.at(1))};
    }
    if (settings.has("presentMode")) {
        config.presentMode = toPresentMode(settings.getString("presentMode", ""));
    }
    return config;
}

}  // namespace

void registerRenderer(plugins::backend::BackendRegistry& registry) {
    registry.addRenderer("vulkan",
                         [](const plugins::backend::config::Settings& settings)
                             -> std::unique_ptr<plugins::backend::graphics::IRenderer> {
                             return std::make_unique<VulkanRenderer>(toConfig(settings));
                         });
}

}  // namespace rtype::render::vulkan
