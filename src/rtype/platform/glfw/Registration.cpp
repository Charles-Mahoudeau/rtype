/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "Registration.hpp"

#include <memory>

#include "GlfwPlatform.hpp"
#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/platform/IPlatform.hpp"

namespace rtype::platform::glfw {

void registerPlatform(engine::backend::BackendRegistry& registry) {
    registry.addPlatform("glfw",
                         [](const engine::config::Settings& settings) -> std::unique_ptr<engine::platform::IPlatform> {
                             settings.checkKeys({}, "glfw");
                             return std::make_unique<GlfwPlatform>();
                         });
}

}  // namespace rtype::platform::glfw
