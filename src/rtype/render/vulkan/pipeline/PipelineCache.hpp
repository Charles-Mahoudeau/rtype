/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineCache
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief The pipeline cache shared by every pipeline the renderer builds: pipelines with the same shaders and state
/// reuse the driver's compiled code instead of compiling it again.
///
/// @details Pass getCache() to GraphicsPipeline::Builder::build() and ComputePipeline. getData() can be saved to disk
/// and given back to the constructor at the next launch, so the first frames do not compile every pipeline again (the
/// driver ignores data from another GPU or driver version).
/// @warning Destroy it before the Device.
class RTYPE_RENDER_VULKAN_API PipelineCache {
  public:
    /// @param device The device the cache is created on.
    /// @param initialData Data of a previous getData(), or nothing to start empty.
    /// @throws vk::SystemError If the cache cannot be created.
    explicit PipelineCache(const core::Device& device, std::span<const std::byte> initialData = {});
    ~PipelineCache() = default;
    PipelineCache(const PipelineCache&) = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;
    PipelineCache(PipelineCache&&) noexcept = default;
    PipelineCache& operator=(PipelineCache&&) noexcept = default;

    /// @return The Vulkan cache, for the pipeline builders.
    [[nodiscard]] const vk::raii::PipelineCache& getCache() const noexcept { return _cache; }

    /// @return The cache's content, to save and give back to the constructor at the next launch.
    [[nodiscard]] std::vector<std::uint8_t> getData() const;

  private:
    vk::raii::PipelineCache _cache;  ///< Destroyed on destruction.
};

}  // namespace rtype::render::vulkan::pipeline
