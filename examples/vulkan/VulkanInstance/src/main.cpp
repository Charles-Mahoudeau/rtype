/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"
#include "platform/Window.hpp"
#include "vulkan/core/DebugMessenger.hpp"
#include "vulkan/core/Instance.hpp"

using namespace rtype::engine::event;
using rtype::engine::input::Key;
using rtype::platform::Window;
using rtype::vulkan::core::DebugMessenger;
using rtype::vulkan::core::Instance;

/// @warning FOR MACOS ONLY: brew install molten-vk vulkan-loader

int main() {
    try {
        /// @note 1. Share one Vulkan loader. The renderer is linked against a loader: hand its entry point to
        /// GLFW BEFORE the first Window is created, otherwise GLFW dlopen()s a loader of its own, which may be
        /// another one, or none at all (macOS).
        Window::initVulkanLoader(Instance::getLoaderEntryPoint());

        /// @note 2. Create the window. It initializes GLFW, which is needed to know the surface extensions.
        Window window(800, 600, "Vulkan Instance Example");

        /// @note 3. Ask the window which instance extensions its surface needs (VK_KHR_surface plus the platform
        /// one: VK_EXT_metal_surface, VK_KHR_xcb_surface, VK_KHR_win32_surface...). Vulkan stays out of the
        /// platform module: it only hands over names.
        std::vector<const char*> extensions = Window::getRequiredVulkanExtensions();

        /// @note 4. Choose the layers. Validation layers are a debug tool: enable them in debug builds only
        /// (xmake f -m debug). Instance throws if a requested layer is not installed (brew install
        /// vulkan-validationlayers on macOS). The DebugMessenger, which prints their messages, needs its
        /// extension too.
        std::vector<const char*> layers;
#ifndef NDEBUG
        layers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back(DebugMessenger::kExtensionName);
#endif

        /// @note 5. Create the instance. Layers and extensions are fixed from now on: a Vulkan instance cannot
        /// change them after its creation. Portability (MoltenVK) is enabled by Instance itself when needed.
        const Instance instance("Vulkan Instance Example", "R-Type Engine", VK_API_VERSION_1_3, std::move(extensions),
                                std::move(layers));

        /// @note 6. Print the validation messages, from warnings up. Pass eInfo or eVerbose to see more. The
        /// messenger only exists in debug builds: std::optional keeps one code path for both.
        std::optional<DebugMessenger> debugMessenger;
#ifndef NDEBUG
        debugMessenger.emplace(instance, vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
#endif

        /// @note The instance works: list the GPUs it can see.
        for (const auto& device : instance.getInstance().enumeratePhysicalDevices()) {
            std::cout << "GPU: " << device.getProperties().deviceName << "\n";
        }
        std::cout << std::flush;

        /// @note Next steps (not done here): the surface, created from instance and window.getNativeHandle(),
        /// then the device and the swapchain.
        while (window.isOpen()) {
            for (const auto& event : window.pollEvents()) {
                if (const auto* key = std::get_if<KeyPressed>(&event); key != nullptr && key->key == Key::kEscape) {
                    window.close();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
