/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePass
*/

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "MaterialBinding.hpp"
#include "ShapeBatch.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief Draws a frame's ShapeBatch: one instanced draw of a quad per batch, with premultiplied alpha (each shape's
/// graphics::BlendMode is read per instance by the shaders).
///
/// @details Owns the pipeline of the default shader (shaders/shape.vert and shape.frag, on the shared layout) and, per
/// frame in flight, an instance buffer and a buffer of the game materials' instance values, CPU-written and read by the
/// GPU as per-instance vertex attributes, like SpritePass. The kind of each shape travels in its instance, so
/// rectangles, circles and capsules of any number draw in one call. A frame's buffers are only used by that frame,
/// whose fence was waited for before recording, so they are rewritten, or replaced by bigger ones, without waiting.
///
/// A batch with a game material is drawn by its shader's pipeline (createMaterialPipeline(), on the game shaders'
/// layout), with its set 2 and, as a shape has no texture, the set 1 given to record(). If the shader has instance
/// values, they are a second vertex binding, locations 8 to 11, and its draw starts at instance 0 with both buffers
/// bound at the batch's first entries, since the two arrays are not indexed alike.
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

    /// @return The pipeline of a game shader's shape variant: @p vertex and @p fragment, the ShapeInstance binding and,
    /// if @p instanceValues, the instance values binding, premultiplied alpha, on @p layout (the game shaders' layout).
    /// @throws vk::SystemError If the pipeline cannot be created.
    [[nodiscard]] static pipeline::GraphicsPipeline createMaterialPipeline(
        const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
        bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
        const vk::raii::PipelineCache* cache);

    /// @brief Copies @p shapes into frame @p frameIndex's buffers and records their draws, inside dynamic rendering,
    /// into @p viewport: a batch with a game material through @p findMaterial (the default material if it gives no
    /// pipeline). The frame's set 0 must already be bound. Nothing is recorded without shapes.
    /// @param textureSet The set 1 of the batches with a game material: what TEXTURE reads in their shader.
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const ShapeBatch& shapes,
                const plugins::backend::graphics::Viewport& viewport, vk::DescriptorSet textureSet,
                const MaterialFinder& findMaterial);

  private:
    /// @brief Instances the first buffer holds; each new one doubles until the frame's instances fit.
    static constexpr std::size_t kInitialCapacity = 256;

    /// @return @p buffer, replaced by a bigger one if it holds fewer than @p count elements of @p elementSize bytes.
    memory::Buffer& reserve(std::optional<memory::Buffer>& buffer, std::size_t count, std::size_t elementSize,
                            std::string_view name);

    const memory::Allocator* _allocator;                  ///< Allocates the instance buffers; not owned.
    pipeline::GraphicsPipeline _pipeline;                 ///< Shape shaders, instanced input, premultiplied alpha.
    std::vector<std::optional<memory::Buffer>> _buffers;  ///< One instance buffer per frame in flight.
    std::vector<std::optional<memory::Buffer>> _values;   ///< One instance values buffer per frame in flight.
};

}  // namespace rtype::render::vulkan::rendering
