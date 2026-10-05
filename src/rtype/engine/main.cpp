/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/event/Event.hpp"
#include "engine/input/Input.hpp"
#include "engine/input/Key.hpp"
#include "platform/glfw/Registration.hpp"
#include "render/vulkan/Registration.hpp"

int main() {
    try {
        rtype::engine::backend::BackendRegistry registry;
        rtype::platform::glfw::registerPlatform(registry);
        rtype::render::vulkan::registerRenderer(registry);

        rtype::engine::config::Settings config;

        config.set("platform", "glfw");
        config.set("renderer", "vulkan");

        config.set("window.title", "R-Type");
        config.set("window.size", std::vector<double>{800, 600});
        config.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        config.set("vulkan.debugging", true);

        const rtype::engine::backend::Backend backend = registry.createBackend(config);
        auto& platform = *backend.platform;
        auto& renderer = *backend.renderer;

        rtype::engine::input::Input input;
        while (platform.isOpen()) {
            for (const auto& event : platform.pollEvents()) {
                input.handleEvent(event);
                if (const auto* resized = std::get_if<rtype::engine::event::Resized>(&event); resized != nullptr) {
                    renderer.resize({resized->width, resized->height});
                }
                if (const auto* key = std::get_if<rtype::engine::event::KeyPressed>(&event);
                    key != nullptr && key->key == rtype::engine::input::Key::kEscape) {
                    platform.close();
                }
            }
            input.update();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
