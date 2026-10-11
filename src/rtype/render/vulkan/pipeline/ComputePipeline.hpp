/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComputePipeline
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "PipelineLayout.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief A compute pipeline: one compute shader (entry point "main") and its layout. Post-processing, particles,
/// culling...
/// @warning Destroy it before the Device. Its PipelineLayout must outlive it.
class RTYPE_RENDER_VULKAN_API ComputePipeline {
  public:
    /// @param device The device the pipeline is created on.
    /// @param shader The compute shader; only needed during the construction.
    /// @param layout Its descriptor sets and push constants; must outlive the pipeline.
    /// @param specialization Specialization constants of the shader, or null.
    /// @param cache Shared pipeline cache to speed up creation, or null.
    /// @throws vk::SystemError If the pipeline cannot be created.
    ComputePipeline(const core::Device& device, const vk::raii::ShaderModule& shader, const PipelineLayout& layout,
                    const vk::SpecializationInfo* specialization = nullptr,
                    const vk::raii::PipelineCache* cache = nullptr);
    ~ComputePipeline() = default;
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&&) noexcept = default;
    ComputePipeline& operator=(ComputePipeline&&) noexcept = default;

    /// @return The Vulkan pipeline.
    [[nodiscard]] const vk::raii::Pipeline& getPipeline() const noexcept { return _pipeline; }
    /// @return The layout it was built with, to bind descriptor sets and push constants.
    [[nodiscard]] const PipelineLayout& getLayout() const noexcept { return *_layout; }

    /// @brief Records the binding of this pipeline into @p commandBuffer, before dispatch().
    void bind(const vk::raii::CommandBuffer& commandBuffer) const;

  private:
    vk::raii::Pipeline _pipeline;   ///< Destroyed on destruction.
    const PipelineLayout* _layout;  ///< Not owned.
};

}  // namespace rtype::render::vulkan::pipeline
