/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Instance
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::vulkan::core {
class Instance {
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

  protected:
  private:
    vk::raii::Context _context;
    vk::raii::Instance _instance = nullptr;

    static bool isExtensionAvailable(std::span<const vk::ExtensionProperties> available, std::string_view name);

    static bool isLayerAvailable(std::span<const vk::LayerProperties> available, std::string_view name);
};
}  // namespace rtype::vulkan::core
