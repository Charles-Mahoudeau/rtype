/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <glm/ext/vector_uint2.hpp>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/event/Event.hpp"
#include "engine/graphics/Color.hpp"
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
#ifndef NDEBUG
        // Debug builds only: the validation layer (with synchronization validation) is slow.
        config.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        config.set("vulkan.debugging", true);
#endif

        const rtype::engine::backend::Backend backend = registry.createBackend(config);
        auto& platform = *backend.platform;
        auto& renderer = *backend.renderer;

        rtype::engine::input::Input input;
        while (platform.isOpen()) {
            // Minimized (0x0 framebuffer): nothing to render, so sleep until an event (restore, close) arrives.
            const glm::uvec2 framebufferSize = platform.getFramebufferSize();
            const bool minimized = framebufferSize.x == 0 || framebufferSize.y == 0;
            for (const auto& event : minimized ? platform.waitEvents() : platform.pollEvents()) {
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
            if (!minimized) {
                renderer.beginFrame({0.05F, 0.05F, 0.15F, 1.0F});
                renderer.endFrame();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
