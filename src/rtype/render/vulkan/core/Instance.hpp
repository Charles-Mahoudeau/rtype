/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Instance
*/

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {
/// @brief Extra checks of the validation layer, enabled through VK_EXT_layer_settings.
/// @details Only applied when Instance::kValidationLayerName is among the layers: without the layer, there is nothing
/// to configure, so release builds (which do not load it) ignore them. Outside Instance so it can have default member
/// initializers and still be a default argument of its constructor.
struct ValidationOptions {
    bool synchronization = false;  ///< Synchronization validation: missing barriers, hazards. Slow.
    bool bestPractices = false;    ///< Warnings about valid but inefficient or risky API usage.
};

class RTYPE_RENDER_VULKAN_API Instance {
  public:
    /// @brief The Khronos validation layer, the one ValidationOptions configures.
    static constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

    /// @brief Creates a Vulkan instance with the specified application and engine names, API version, required
    /// extensions, and layers.
    /// @param appName Name of the application.
    /// @param engineName Name of the engine.
    /// @param apiVersion Vulkan API version to use.
    /// @param requiredExtensions Instance extensions to enable, e.g. the ones the window needs for its surface.
    /// Taken by value: VK_KHR_portability_enumeration is appended when the loader offers it (MoltenVK), and
    /// VK_EXT_layer_settings when @p validation enables something.
    /// @param layers Instance layers to enable.
    /// @param validation Extra checks of the validation layer, ignored when it is not in @p layers.
    /// @param messenger Debug messenger active during vkCreateInstance and vkDestroyInstance, whose messages the
    /// DebugMessenger cannot catch (e.g. objects leaked at exit): DebugMessenger::makeCreateInfo(). Requires
    /// VK_EXT_debug_utils in @p requiredExtensions.
    /// @throws std::invalid_argument If @p messenger is given without VK_EXT_debug_utils in @p requiredExtensions.
    /// @throws std::runtime_error If one of the required extensions is not supported.
    /// @throws std::runtime_error If one of the layers is not supported.
    /// @throws std::runtime_error If @p validation enables something the validation layer does not offer.
    Instance(const std::string_view& appName, const std::string_view& engineName, uint32_t apiVersion,
             std::vector<const char*> requiredExtensions, std::vector<const char*> layers = {},
             ValidationOptions validation = {},
             const std::optional<vk::DebugUtilsMessengerCreateInfoEXT>& messenger = std::nullopt);
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

    /// @brief Resolves the validation options to apply: none when the validation layer is not in @p layers or
    /// nothing is enabled; otherwise appends VK_EXT_layer_settings to @p extensions.
    /// @throws std::runtime_error If the validation layer does not offer VK_EXT_layer_settings.
    [[nodiscard]] ValidationOptions enableValidationFeatures(ValidationOptions requested,
                                                             std::span<const char* const> layers,
                                                             std::vector<const char*>& extensions) const;

    /// @brief Creates the instance from the already checked extensions and layers, with @p messenger and the
    /// validation layer settings of @p validation chained into its pNext.
    [[nodiscard]] vk::raii::Instance createInstance(
        const vk::ApplicationInfo& appInfo, vk::InstanceCreateFlags flags, std::span<const char* const> extensions,
        std::span<const char* const> layers, ValidationOptions validation,
        const std::optional<vk::DebugUtilsMessengerCreateInfoEXT>& messenger) const;

    vk::raii::Context _context;              ///< Loader dispatch, built from getLoaderEntryPoint().
    vk::raii::Instance _instance = nullptr;  ///< The Vulkan instance, created by the constructor.
};
}  // namespace rtype::render::vulkan::core
