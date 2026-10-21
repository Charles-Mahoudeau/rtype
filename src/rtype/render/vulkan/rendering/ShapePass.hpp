/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePass
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "ShapeInstance.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief Draws a frame's shapes: every one of them in a single instanced draw of a quad, alpha blended.
///
/// @details Owns the shape pipeline (shaders/shape.vert and shape.frag, on the shared layout) and one instance buffer
/// per frame in flight, CPU-written and read by the GPU as per-instance vertex attributes, like SpritePass. The kind of
/// each shape travels in its instance, so rectangles, circles and capsules of any number draw in one call. A frame's
/// buffer is only used by that frame, whose fence was waited for before recording, so it is rewritten, or replaced by a
/// bigger one, without waiting.
/// @warning Destroy it before the Allocator and the Device.
class RTYPE_RENDER_VULKAN_API ShapePass {
  public:
    /// @param shaders Gives the built-in shape shaders.
    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the swapchain's.
    /// @param cache Speeds up the pipeline's creation, or null.
    /// @throws vk::SystemError If the pipeline cannot be created.
    ShapePass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
              const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache,
              std::size_t framesInFlight);
    ~ShapePass() = default;
    ShapePass(const ShapePass&) = delete;
    ShapePass& operator=(const ShapePass&) = delete;
    ShapePass(ShapePass&&) = delete;
    ShapePass& operator=(ShapePass&&) = delete;

    /// @brief Copies @p shapes into frame @p frameIndex's buffer and records their single draw, inside dynamic
    /// rendering, into @p viewport. The frame's set 0 must already be bound. Nothing is recorded without shapes.
    /// @throws vk::SystemError If a bigger instance buffer cannot be created.
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                std::span<const ShapeInstance> shapes, const plugins::backend::graphics::Viewport& viewport);

  private:
    /// @brief Instances the first buffer holds; each new one doubles until the frame's instances fit.
    static constexpr std::size_t kInitialCapacity = 256;

    /// @return Frame @p frameIndex's buffer, replaced by a bigger one if it holds fewer than @p count instances.
    memory::Buffer& reserve(std::size_t frameIndex, std::size_t count);

    const memory::Allocator* _allocator;                  ///< Allocates the instance buffers; not owned.
    pipeline::GraphicsPipeline _pipeline;                 ///< Shape shaders, instanced input, alpha blending.
    std::vector<std::optional<memory::Buffer>> _buffers;  ///< One instance buffer per frame in flight.
};

}  // namespace rtype::render::vulkan::rendering
