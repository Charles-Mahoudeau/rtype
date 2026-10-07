/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Instance
*/

#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {
class RTYPE_RENDER_VULKAN_API Instance {
  public:
    /// @brief Creates a Vulkan instance with the specified application and engine names, API version, required
    /// extensions, and layers.
    /// @param appName Name of the application.
    /// @param engineName Name of the engine.
    /// @param apiVersion Vulkan API version to use.
    /// @param requiredExtensions Instance extensions to enable, e.g. the ones the window needs for its surface.
    /// Taken by value: VK_KHR_portability_enumeration is appended when the loader offers it (MoltenVK).
    /// @param layers Instance layers to enable.
    /// @throws std::runtime_error If one of the required extensions is not supported.
    /// @throws std::runtime_error If one of the layers is not supported.
    Instance(const std::string_view& appName, const std::string_view& engineName, uint32_t apiVersion,
             std::vector<const char*> requiredExtensions, std::vector<const char*> layers = {});
    ~Instance() = default;
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&&) = delete;
    Instance& operator=(Instance&&) = delete;

    /// @return The Vulkan instance handle.
    [[nodiscard]] const vk::raii::Instance& getInstance() const;

    /// @return The Vulkan context handle.
    [[nodiscard]] const vk::raii::Context& getContext() const;

    /// @return The entry point of the Vulkan loader this library is linked against.
    /// @note Hand it to every other Vulkan user (e.g. GLFW) so the whole process shares one loader.
    [[nodiscard]] static PFN_vkGetInstanceProcAddr getLoaderEntryPoint() noexcept;

  private:
    /// @throws std::runtime_error Naming the first required extension the loader does not offer.
    static void checkExtensionsSupported(std::span<const vk::ExtensionProperties> available,
                                         std::span<const char* const> required);

    /// @throws std::runtime_error Naming the first requested layer the loader does not offer.
    static void checkLayersSupported(std::span<const vk::LayerProperties> available,
                                     std::span<const char* const> requested);

    /// @brief Opts in to portability drivers (MoltenVK on macOS) when the loader offers it.
    /// @details Appends VK_KHR_portability_enumeration to the extensions, which must go with the returned flag:
    /// without both, the loader hides portability drivers and instance creation fails.
    /// @return The instance creation flags to use, empty when the loader does not offer the extension.
    static vk::InstanceCreateFlags enablePortability(std::span<const vk::ExtensionProperties> available,
                                                     std::vector<const char*>& extensions);

    /// @return True if the loader offers the instance extension.
    static bool isExtensionAvailable(std::span<const vk::ExtensionProperties> available, std::string_view name);

    /// @return True if the loader offers the instance layer.
    static bool isLayerAvailable(std::span<const vk::LayerProperties> available, std::string_view name);

    /// @brief Creates the instance from the already checked extensions and layers.
    [[nodiscard]] vk::raii::Instance createInstance(const vk::ApplicationInfo& appInfo, vk::InstanceCreateFlags flags,
                                                    std::span<const char* const> extensions,
                                                    std::span<const char* const> layers) const;

    vk::raii::Context _context;              ///< Loader dispatch, built from getLoaderEntryPoint().
    vk::raii::Instance _instance = nullptr;  ///< The Vulkan instance, created by the constructor.
};
}  // namespace rtype::render::vulkan::core
