/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineLayout
*/

#pragma once

#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief What a pipeline's shaders can read besides their vertices: descriptor sets and push constants.
///
/// @details Shared by every pipeline whose shaders declare the same sets and push constants, and needed again when
/// recording, to bind descriptor sets and push constants.
/// @warning Destroy it before the Device. The pipelines only need it while they are created.
class RTYPE_RENDER_VULKAN_API PipelineLayout {
  public:
    /// @param device The device the layout is created on.
    /// @param setLayouts The descriptor set layouts, in set order (set 0 first), e.g. DescriptorSetLayout handles.
    /// @param pushConstantRanges The push constant ranges, by stage.
    /// @throws vk::SystemError If the layout cannot be created.
    explicit PipelineLayout(const core::Device& device, std::span<const vk::DescriptorSetLayout> setLayouts = {},
                            std::span<const vk::PushConstantRange> pushConstantRanges = {});
    ~PipelineLayout() = default;
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;
    PipelineLayout(PipelineLayout&&) noexcept = default;
    PipelineLayout& operator=(PipelineLayout&&) noexcept = default;

    /// @return The Vulkan layout.
    [[nodiscard]] const vk::raii::PipelineLayout& getLayout() const noexcept { return _layout; }
    /// @return The push constant ranges it was created with.
    [[nodiscard]] std::span<const vk::PushConstantRange> getPushConstantRanges() const noexcept {
        return _pushConstantRanges;
    }

  private:
    vk::raii::PipelineLayout _layout;                        ///< Destroyed on destruction.
    std::vector<vk::PushConstantRange> _pushConstantRanges;  ///< Copy of the ranges.
};

}  // namespace rtype::render::vulkan::pipeline
