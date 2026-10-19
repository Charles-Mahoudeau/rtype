/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "platform/glfw/Registration.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace {
/// @brief Stops the App when Escape is pressed.
void quitOnEscape(rtype::ecs::Res<rtype::plugins::input::Input> input,
                  rtype::ecs::EventWriter<rtype::ecs::AppExit> exit) {
    if (input->read(rtype::plugins::input::Control::key(rtype::plugins::backend::input::Key::kEscape)) > 0.0F) {
        exit.send(rtype::ecs::AppExit{.code = 0});
    }
}

/// @return The configuration: which platform and renderer to start, and how.
rtype::plugins::backend::config::Settings makeSettings() {
    rtype::plugins::backend::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "R-Type");
    settings.set("window.size", std::vector<double>{800, 600});
#ifndef NDEBUG
    // Debug builds only: the validation layer (with synchronization validation) is slow.
    settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
    settings.set("vulkan.debugging", true);
#endif
    return settings;
}
}  // namespace

int main() {
    try {
        // The only place that knows GLFW and Vulkan: the plugins pick them by name from the settings.
        auto registry = std::make_unique<rtype::plugins::backend::BackendRegistry>();
        rtype::platform::glfw::registerPlatform(*registry);
        rtype::render::vulkan::registerRenderer(*registry);

        rtype::ecs::App app;
        app.addPlugins(rtype::ecs::MinimalPlugins{})
            .addPlugin(rtype::plugins::backend::BackendPlugin{std::move(registry), makeSettings()});
        app.addSystem(rtype::ecs::Phase::kUpdate, "rtype.quitOnEscape", &quitOnEscape)
            .setSide(rtype::ecs::Side::kClient);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
