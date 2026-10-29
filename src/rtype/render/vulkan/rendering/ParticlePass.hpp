/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticlePass
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "ParticleConstants.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief Draws particles: one instanced draw of a unit quad per ParticleDraw, with premultiplied alpha, shaded as the
/// sprites are (shaders/particle.vert and sprite.frag, on the shared layout).
///
/// @details Owns the particle pipeline and one instance buffer per frame in flight, into which upload() copies every
/// CPU particle of a frame at once; each draw then reads its range of it. A frame's buffer is only used by that frame,
/// whose fence was waited for before recording, so it is rewritten, or replaced by a bigger one, without waiting. A GPU
/// system will be drawn by the same pipeline, from the buffer its simulation packs.
/// @warning Destroy it before the Allocator and the Device.
class RTYPE_RENDER_VULKAN_API ParticlePass {
  public:
    /// @brief One draw: a range of the uploaded instances, with what they share.
    struct Draw {
        ParticleConstants constants;      ///< Uv rect, blend mode and alignment.
        vk::DescriptorSet textureSet;     ///< The texture's set; not owned.
        std::uint32_t firstInstance = 0;  ///< First instance of the range, in the uploaded instances.
        std::uint32_t instanceCount = 0;  ///< Number of instances.
    };

    /// @param shaders Gives the built-in particle and sprite shaders.
    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the swapchain's.
    /// @param cache Speeds up the pipeline's creation, or null.
    /// @throws vk::SystemError If the pipeline cannot be created.
    ParticlePass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                 const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache,
                 std::size_t framesInFlight);
    ~ParticlePass() = default;
    ParticlePass(const ParticlePass&) = delete;
    ParticlePass& operator=(const ParticlePass&) = delete;
    ParticlePass(ParticlePass&&) = delete;
    ParticlePass& operator=(ParticlePass&&) = delete;

    /// @brief Copies @p instances, every CPU particle of the frame, into frame @p frameIndex's buffer, for record().
    /// @throws vk::SystemError If a bigger instance buffer cannot be created.
    void upload(std::size_t frameIndex, std::span<const plugins::backend::graphics::ParticleInstance> instances);

    /// @brief Records @p draw, inside dynamic rendering, into @p viewport: binds the pipeline (so call it again after
    /// another pass recorded), then draws its range of frame @p frameIndex's buffer. The frame's set 0 must already be
    /// bound. Does nothing for an empty range, a null texture set or before upload().
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const Draw& draw,
                const plugins::backend::graphics::Viewport& viewport);

  private:
    /// @brief Instances the first buffer holds; each new one doubles until the frame's instances fit.
    static constexpr std::size_t kInitialCapacity = 1024;

    /// @return Frame @p frameIndex's buffer, replaced by a bigger one if it holds fewer than @p count instances.
    memory::Buffer& reserve(std::size_t frameIndex, std::size_t count);

    const memory::Allocator* _allocator;                  ///< Allocates the instance buffers; not owned.
    const pipeline::PipelineLayout* _layout;              ///< The shared layout; not owned.
    pipeline::GraphicsPipeline _pipeline;                 ///< Particle and sprite shaders, premultiplied alpha.
    std::vector<std::optional<memory::Buffer>> _buffers;  ///< One instance buffer per frame in flight.
};

}  // namespace rtype::render::vulkan::rendering
