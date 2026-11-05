/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandLabel
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::command {

/// @brief Labels the commands recorded during its lifetime, so that a GPU capture shows them as one named group.
///
/// @details Does nothing without VK_EXT_debug_utils (core::Device::hasDebugUtils()). Labels can be nested.
/// @code
/// const command::CommandLabel label{device, commandBuffer, "Sprites"};
/// spritePass.record(commandBuffer, ...);
/// @endcode
class RTYPE_RENDER_VULKAN_API CommandLabel {
  public:
    /// @param commandBuffer The command buffer being recorded; must outlive the label.
    /// @param name The name of the group.
    CommandLabel(const core::Device& device, const vk::raii::CommandBuffer& commandBuffer, const char* name);
    ~CommandLabel();
    CommandLabel(const CommandLabel&) = delete;
    CommandLabel& operator=(const CommandLabel&) = delete;
    CommandLabel(CommandLabel&&) = delete;
    CommandLabel& operator=(CommandLabel&&) = delete;

  private:
    const vk::raii::CommandBuffer* _commandBuffer = nullptr;  ///< Not owned; null without VK_EXT_debug_utils.
};

}  // namespace rtype::render::vulkan::command
