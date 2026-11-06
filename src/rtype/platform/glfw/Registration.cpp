/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "Registration.hpp"

#include <memory>

#include "GlfwPlatform.hpp"
#include "core/config/Settings.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"

namespace rtype::platform::glfw {

void registerPlatform(contract::platform::PlatformRegistry& registry) {
    registry.add("glfw", [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
        settings.checkKeys({}, "glfw");
        return std::make_unique<GlfwPlatform>();
    });
}

}  // namespace rtype::platform::glfw
