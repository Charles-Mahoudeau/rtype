/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <memory>
#include <variant>

#include "engine/event/Event.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/input/Input.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"
#include "platform/GlfwPlatform.hpp"
#include "vulkan/VulkanRenderer.hpp"

int main() {
    try {
        const std::unique_ptr<rtype::engine::platform::IPlatform> platform =
            std::make_unique<rtype::platform::GlfwPlatform>();
        const std::unique_ptr<rtype::engine::graphics::IRenderer> renderer =
            std::make_unique<rtype::vulkan::VulkanRenderer>(
                rtype::vulkan::VulkanRenderer::Config{.layers = {"VK_LAYER_KHRONOS_validation"}, .debugging = true});

        platform->initLoader(renderer->getLoaderEntryPoint());
        platform->init({.size = {800, 600}, .title = "R-Type", .resizable = true, .fullscreen = false});
        renderer->init(*platform);

        rtype::engine::input::Input input;
        while (platform->isOpen()) {
            for (const auto& event : platform->pollEvents()) {
                input.handleEvent(event);
                if (const auto* resized = std::get_if<rtype::engine::event::Resized>(&event); resized != nullptr) {
                    renderer->resize({resized->width, resized->height});
                }
                if (const auto* key = std::get_if<rtype::engine::event::KeyPressed>(&event);
                    key != nullptr && key->key == rtype::engine::input::Key::kEscape) {
                    platform->close();
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
