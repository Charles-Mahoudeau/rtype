/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePass
*/

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "SpriteBatch.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief Draws a frame's SpriteBatch: one instanced draw of a unit quad per batch, alpha blended.
///
/// @details Owns the sprite pipeline (shaders/sprite.vert and sprite.frag, on the shared layout) and one instance
/// buffer per frame in flight, CPU-written and read by the GPU as per-instance vertex attributes. A frame's buffer is
/// only used by that frame, whose fence was waited for before recording, so it is rewritten, or replaced by a bigger
/// one, without waiting.
/// @warning Destroy it before the Allocator and the Device.
class RTYPE_RENDER_VULKAN_API SpritePass {
  public:
    /// @brief Finds the descriptor set of a texture, or a null handle if it no longer exists.
    using TextureSetFinder = std::function<vk::DescriptorSet(plugins::backend::graphics::TextureId)>;

    /// @param shaders Gives the built-in sprite shaders.
    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the swapchain's.
    /// @param cache Speeds up the pipeline's creation, or null.
    /// @throws vk::SystemError If the pipeline cannot be created.
    SpritePass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
               const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache,
               std::size_t framesInFlight);
    ~SpritePass() = default;
    SpritePass(const SpritePass&) = delete;
    SpritePass& operator=(const SpritePass&) = delete;
    SpritePass(SpritePass&&) = delete;
    SpritePass& operator=(SpritePass&&) = delete;

    /// @brief Copies @p batch's instances into frame @p frameIndex's buffer and records its draws, inside dynamic
    /// rendering, into @p viewport. The frame's set 0 must already be bound. A batch whose texture no longer exists is
    /// skipped.
    /// @throws vk::SystemError If a bigger instance buffer cannot be created.
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const SpriteBatch& batch,
                const plugins::backend::graphics::Viewport& viewport, const TextureSetFinder& findTextureSet);

  private:
    /// @brief Instances the first buffer holds; each new one doubles until the frame's instances fit.
    static constexpr std::size_t kInitialCapacity = 1024;

    /// @return Frame @p frameIndex's buffer, replaced by a bigger one if it holds fewer than @p count instances.
    memory::Buffer& reserve(std::size_t frameIndex, std::size_t count);

    const memory::Allocator* _allocator;                  ///< Allocates the instance buffers; not owned.
    const pipeline::PipelineLayout* _layout;              ///< The shared layout; not owned.
    pipeline::GraphicsPipeline _pipeline;                 ///< Sprite shaders, instanced input, alpha blending.
    std::vector<std::optional<memory::Buffer>> _buffers;  ///< One instance buffer per frame in flight.
};

}  // namespace rtype::render::vulkan::rendering
