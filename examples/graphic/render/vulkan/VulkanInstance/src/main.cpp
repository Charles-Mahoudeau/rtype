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
#include "engine/input/Key.hpp"
#include "platform/glfw/Registration.hpp"
#include "render/vulkan/Registration.hpp"

using namespace rtype::engine::event;
using rtype::engine::backend::Backend;
using rtype::engine::backend::BackendRegistry;
using rtype::engine::config::Settings;
using rtype::engine::input::Key;

/// @warning FOR MACOS ONLY: brew install molten-vk vulkan-loader (+ vulkan-validationlayers in debug builds)

int main() {
    try {
        /// @note 1. Register the backends. Each module adds its own under a name: "glfw" for the platform module,
        /// "vulkan" for the renderer module. The code never names a backend class.
        BackendRegistry registry;
        rtype::platform::glfw::registerPlatform(registry);
        rtype::render::vulkan::registerRenderer(registry);

        /// @note 2. The config picks the pair and sets them up. "window" is for every platform; "vulkan" only for
        /// the vulkan renderer (see rtype::render::vulkan::registerRenderer). Filled in code here; a config file later.
        Settings config;
        config.set("platform", "glfw");
        config.set("renderer", "vulkan");
        config.set("window.title", "Vulkan Instance Example");
        config.set("window.size", std::vector<double>{800, 600});
#ifndef NDEBUG
        /// @note Validation layers are a debug tool (xmake f -m debug). A layer that is not installed makes the
        /// start-up throw (brew install vulkan-validationlayers on macOS).
        config.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        config.set("vulkan.debugging", true);
#endif

        /// @note 3. Start them. createBackend() runs the engine's sequence:
        /// - renderer->prepare(*platform): the renderer finds the platform's IVulkanSurfaceSource (GlfwPlatform
        ///   implements it) and calls initLoader(): GLFW will use the renderer's Vulkan loader;
        /// - platform->init(window): the window, without graphics context;
        /// - renderer->init(*platform): the instance (+ layers, messenger), then the window surface, through
        ///   IVulkanSurfaceSource::getRequiredExtensions() and createSurface().
        /// A wrong name, an unknown setting or a pair that does not fit throws, with a message saying why.
        const Backend backend = registry.createBackend(config);
        std::cout << "Vulkan instance and window surface created.\n" << std::flush;

        /// @note 4. The loop only sees IPlatform and IRenderer. Drawing is not implemented in Vulkan yet.
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
