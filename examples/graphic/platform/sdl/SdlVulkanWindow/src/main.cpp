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
#include <variant>
#include <vector>

#include "SdlPlatform.hpp"
#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"
#include "vulkan/RendererRegistration.hpp"

using namespace rtype::engine::event;
using example::SdlPlatform;
using rtype::engine::backend::Backend;
using rtype::engine::backend::BackendRegistry;
using rtype::engine::config::Settings;
using rtype::engine::input::Key;
using rtype::engine::platform::IPlatform;

/// @warning macOS: brew install molten-vk vulkan-loader (+ vulkan-validationlayers in debug builds)

int main() {
    try {
        /// @note Your own platform next to the engine's renderer: the engine registers "vulkan", this example
        /// registers "sdl" (SdlPlatform, written here). The factory receives the "sdl" section of the config; this
        /// platform has no setting, so it only rejects unknown keys.
        BackendRegistry registry;
        rtype::vulkan::registerRenderers(registry);
        registry.addPlatform("sdl", [](const Settings& settings) -> std::unique_ptr<IPlatform> {
            settings.checkKeys({}, "sdl");
            return std::make_unique<SdlPlatform>();
        });

        /// @note Same config as examples/graphic/render/vulkan/VulkanInstance, with "sdl" instead of "glfw". The
        /// VulkanRenderer never knows SDL is behind it: it only uses the IVulkanSurfaceSource SdlPlatform implements.
        Settings config;
        config.set("platform", "sdl");
        config.set("renderer", "vulkan");
        config.set("window.title", "SDL + Vulkan Example");
        config.set("window.size", std::vector<double>{800, 600});
#ifndef NDEBUG
        config.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        config.set("vulkan.debugging", true);
#endif

        /// @note On the SDL side, createBackend() leads to:
        /// 1. renderer->prepare(): the renderer calls initLoader(); SdlPlatform finds the file of the renderer's
        ///    loader, and init() loads it with SDL_Vulkan_LoadLibrary(), so SDL and the renderer share it;
        /// 2. platform->init(): SDL_CreateWindow() with SDL_WINDOW_VULKAN;
        /// 3. renderer->init(): the renderer calls getRequiredExtensions() (SDL_Vulkan_GetInstanceExtensions) and
        ///    createSurface() (SDL_Vulkan_CreateSurface).
        const Backend backend = registry.createBackend(config);
        std::cout << "SDL window with a Vulkan surface created.\n" << std::flush;

        while (backend.platform->isOpen()) {
            for (const auto& event : backend.platform->pollEvents()) {
                if (const auto* resized = std::get_if<Resized>(&event); resized != nullptr) {
                    backend.renderer->resize({resized->width, resized->height});
                }
                if (const auto* key = std::get_if<KeyPressed>(&event); key != nullptr && key->key == Key::kEscape) {
                    backend.platform->close();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
