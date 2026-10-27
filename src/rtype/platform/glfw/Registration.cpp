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
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/platform/IPlatform.hpp"

namespace rtype::platform::glfw {

void registerPlatform(plugins::backend::BackendRegistry& registry) {
    registry.addPlatform(
        "glfw", [](const core::config::Settings& settings) -> std::unique_ptr<plugins::backend::platform::IPlatform> {
            settings.checkKeys({}, "glfw");
            return std::make_unique<GlfwPlatform>();
        });
}

}  // namespace rtype::platform::glfw
