/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "Registration.hpp"

#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "backend/raylib/platform/RaylibPlatform.hpp"
#include "backend/raylib/renderer/RaylibRenderer.hpp"
#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"

namespace rtype::backend::raylib {
namespace {

/// @return The `virtualResolution` of the section "raylib", or nothing.
/// @throws core::config::exceptions::SettingsException If a key is unknown or the resolution is not two positive
/// numbers.
std::optional<glm::uvec2> readVirtualResolution(const core::config::Settings& settings) {
    settings.checkKeys({"virtualResolution"}, "raylib");
    if (!settings.has("virtualResolution")) {
        return std::nullopt;
    }
    const std::vector<double> size = settings.getNumberList("virtualResolution", {});
    if (size.size() != 2 || size.at(0) <= 0.0 || size.at(1) <= 0.0) {
        throw core::config::exceptions::SettingsException(
            "Setting 'raylib.virtualResolution' must be { width, height }, both positive");
    }
    return glm::uvec2{static_cast<unsigned>(size.at(0)), static_cast<unsigned>(size.at(1))};
}

}  // namespace

void registerPlatform(contract::platform::PlatformRegistry& registry) {
    registry.add("raylib",
                 [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
                     static_cast<void>(readVirtualResolution(settings));
                     return std::make_unique<platform::RaylibPlatform>();
                 });
}

void registerRenderer(contract::renderer::RendererRegistry& registry) {
    registry.add("raylib",
                 [](const core::config::Settings& settings) -> std::unique_ptr<contract::renderer::IRenderer> {
                     return std::make_unique<renderer::RaylibRenderer>(readVirtualResolution(settings));
                 });
}

}  // namespace rtype::backend::raylib
