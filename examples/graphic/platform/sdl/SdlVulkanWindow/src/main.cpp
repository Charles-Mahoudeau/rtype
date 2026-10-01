/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

#include "SdlPlatform.hpp"
#include "engine/event/Event.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"
#include "vulkan/VulkanRenderer.hpp"

using namespace rtype::engine::event;
using example::SdlPlatform;
using rtype::engine::graphics::IRenderer;
using rtype::engine::input::Key;
using rtype::engine::platform::IPlatform;
using rtype::vulkan::VulkanRenderer;

/// @warning macOS: brew install sdl3 molten-vk vulkan-loader (+ vulkan-validationlayers in debug builds)

int main() {
    try {
        /// @note The same program as examples/graphic/render/vulkan/VulkanInstance, with SdlPlatform (written in this
        /// example) instead of GlfwPlatform. The engine's VulkanRenderer is used as is: it only talks to IPlatform, so
        /// it never knows SDL is behind it.
        VulkanRenderer::Config config;
#ifndef NDEBUG
        config.layers.push_back("VK_LAYER_KHRONOS_validation");
        config.debugging = true;
#endif
        const std::unique_ptr<IPlatform> platform = std::make_unique<SdlPlatform>();
        const std::unique_ptr<IRenderer> renderer = std::make_unique<VulkanRenderer>(std::move(config));

        /// @note The engine's start-up sequence, unchanged. On the SDL side:
        /// 1. initLoader(): SdlPlatform finds the file of the renderer's loader, SDL_Vulkan_LoadLibrary() loads it;
        /// 2. init(): SDL_CreateWindow() with SDL_WINDOW_VULKAN;
        /// 3. the renderer calls getRequiredExtensions() (SDL_Vulkan_GetInstanceExtensions) and createSurface()
        ///    (SDL_Vulkan_CreateSurface).
        platform->initLoader(renderer->getLoaderEntryPoint());
        platform->init({.size = {800, 600}, .title = "SDL + Vulkan Example"});
        renderer->init(*platform);
        std::cout << "SDL window with a Vulkan surface created.\n" << std::flush;

        while (platform->isOpen()) {
            for (const auto& event : platform->pollEvents()) {
                if (const auto* resized = std::get_if<Resized>(&event); resized != nullptr) {
                    renderer->resize({resized->width, resized->height});
                }
                if (const auto* key = std::get_if<KeyPressed>(&event); key != nullptr && key->key == Key::kEscape) {
                    platform->close();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
