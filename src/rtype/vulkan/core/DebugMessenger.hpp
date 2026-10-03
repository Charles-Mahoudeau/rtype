/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DebugMessenger
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Export.hpp"
#include "Instance.hpp"

namespace rtype::vulkan::core {

/// @brief Prints the messages of the validation layers (and of the loader) to the standard error output.
///
/// @details Without a messenger, the validation layers only write their errors to stdout, with no control over
/// the severity. Typical use: enable VK_LAYER_KHRONOS_validation and kExtensionName in debug builds, create the
/// Instance, then the DebugMessenger.
///
/// @warning The instance must be created with kExtensionName enabled, and must outlive the messenger:
/// declare the DebugMessenger after its Instance.
/// @note Messages emitted while the instance itself is created or destroyed are not caught: the messenger only
/// exists between the two.
class RTYPE_VULKAN_API DebugMessenger {
  public:
    /// The instance extension a DebugMessenger needs (VK_EXT_debug_utils).
    static constexpr const char* kExtensionName = vk::EXTDebugUtilsExtensionName;

    /// @param instance Instance created with kExtensionName enabled.
    /// @param minSeverity Messages below this severity are dropped by Vulkan before reaching the callback.
    /// @throws vk::SystemError If the messenger cannot be created, e.g. kExtensionName is not enabled.
    explicit DebugMessenger(const Instance& instance, vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity =
                                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
    ~DebugMessenger() = default;
    DebugMessenger(const DebugMessenger&) = delete;
    DebugMessenger& operator=(const DebugMessenger&) = delete;
    DebugMessenger(DebugMessenger&&) = delete;
    DebugMessenger& operator=(DebugMessenger&&) = delete;

  private:
    /// @return minSeverity and every severity above it.
    static vk::DebugUtilsMessageSeverityFlagsEXT severitiesFrom(vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity);

    /// @brief Called by Vulkan for each message: prints its severity, types and text.
    /// @return vk::False: the Vulkan call that triggered the message must not be aborted.
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL onMessage(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                      vk::DebugUtilsMessageTypeFlagsEXT types,
                                                      const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                                      void* userData);

    vk::raii::DebugUtilsMessengerEXT _messenger;  ///< Registration of onMessage, removed on destruction.
};
}  // namespace rtype::vulkan::core
