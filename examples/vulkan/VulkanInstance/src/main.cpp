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
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"
#include "platform/GlfwPlatform.hpp"
#include "vulkan/VulkanRenderer.hpp"

using namespace rtype::engine::event;
using rtype::engine::graphics::IRenderer;
using rtype::engine::input::Key;
using rtype::engine::platform::IPlatform;
using rtype::platform::GlfwPlatform;
using rtype::vulkan::VulkanRenderer;

/// @warning FOR MACOS ONLY: brew install molten-vk vulkan-loader (+ vulkan-validationlayers in debug builds)

int main() {
    try {
        /// @note 0. Pick the backends. This is the only place that names them: from here on, the code only sees
        /// IPlatform and IRenderer, and would be the same with any other pair. Validation layers are a debug tool:
        /// enabled in debug builds only (xmake f -m debug).
#ifdef NDEBUG
        constexpr bool kEnableValidation = false;
#else
        constexpr bool kEnableValidation = true;
#endif
        const std::unique_ptr<IPlatform> platform = std::make_unique<GlfwPlatform>();
        const std::unique_ptr<IRenderer> renderer = std::make_unique<VulkanRenderer>(kEnableValidation);

        /// @note 1. Share one loader, BEFORE the window exists. The renderer gives the entry point of the Vulkan
        /// loader it is linked against (vkGetInstanceProcAddr), the platform hands it to GLFW
        /// (glfwInitVulkanLoader). Without it, GLFW dlopen()s a loader of its own, which may be another one, or
        /// none at all (macOS). Both functions are optional: with backends that have no loader, it does nothing.
        platform->initLoader(renderer->getLoaderEntryPoint());

        /// @note 2. Create the window. GLFW creates it without any graphics context (GLFW_NO_API): the renderer
        /// brings its own.
        platform->init({.size = {800, 600}, .title = "Vulkan Instance Example"});

        /// @note 3. Attach the renderer. It asks the platform for what it needs, through the optional functions
        /// of IPlatform:
        /// - getRequiredExtensions(): the instance extensions of the window surface (VK_KHR_surface plus the OS
        ///   one: VK_EXT_metal_surface, VK_KHR_win32_surface...), enabled when the instance is created;
        /// - createSurface(instance): the surface of the window (glfwCreateWindowSurface).
        /// A platform that does not provide them throws UnsupportedFeatureException: the pair does not fit.
        /// In debug builds, the instance also gets the validation layer and the DebugMessenger, which prints its
        /// messages to stderr.
        renderer->init(*platform);
        std::cout << "Vulkan instance and window surface created.\n" << std::flush;

        /// @note 4. The loop: events go to whoever needs them; a resize goes to the renderer, which will rebuild
        /// its swapchain. Drawing (beginFrame, draw, endFrame) is not implemented yet.
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
