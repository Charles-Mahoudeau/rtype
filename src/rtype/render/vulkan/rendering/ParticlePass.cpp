/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticlePass
*/

#include "ParticlePass.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "ParticleConstants.hpp"
#include "ParticleVertexInput.hpp"
#include "ShaderLayout.hpp"
#include "particle.vert.h"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "sprite.frag.h"

namespace rtype::render::vulkan::rendering {

namespace {

/// @brief Vertices of the unit quad: two triangles, built by particle.vert from gl_VertexIndex.
constexpr std::uint32_t kQuadVertexCount = 6;

/// @return The particle pipeline.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          const vk::raii::PipelineCache* cache) {
    const auto bindings = getParticleBindings();
    const auto attributes = getParticleAttributes();
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("particle.vert", rtype_shader_particle_vert),
                    shaders.get("sprite.frag", rtype_shader_sprite_frag))
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setLayout(layout)
        .build(device, cache);
}

}  // namespace

ParticlePass::ParticlePass(const core::Device& device, const memory::Allocator& allocator,
                           pipeline::ShaderCache& shaders, const pipeline::PipelineLayout& layout,
                           vk::Format colorFormat, const vk::raii::PipelineCache* cache, std::size_t framesInFlight)
    : _allocator{&allocator},
      _layout{&layout},
      _pipeline{createPipeline(device, shaders, layout, colorFormat, cache)},
      _buffers(framesInFlight) {}

void ParticlePass::upload(std::size_t frameIndex,
                          std::span<const plugins::backend::graphics::ParticleInstance> instances) {
    if (instances.empty()) {
        return;
    }
    reserve(frameIndex, instances.size()).write(std::as_bytes(instances));
}

void ParticlePass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const Draw& draw,
                          const plugins::backend::graphics::Viewport& viewport) {
    const std::optional<memory::Buffer>& buffer = _buffers.at(frameIndex);
    if (draw.instanceCount == 0 || !draw.textureSet || !buffer || viewport.size.x <= 0.0F || viewport.size.y <= 0.0F) {
        return;
    }
    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(
        0, vk::Viewport{viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y, 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{static_cast<std::int32_t>(viewport.position.x),
                                                        static_cast<std::int32_t>(viewport.position.y)},
                                           vk::Extent2D{static_cast<std::uint32_t>(viewport.size.x),
                                                        static_cast<std::uint32_t>(viewport.size.y)}});
    commandBuffer.bindVertexBuffers(0, *buffer->getBuffer(), vk::DeviceSize{0});
    commandBuffer.pushConstants<ParticleConstants>(
        *_layout->getLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
        draw.constants);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_layout->getLayout(), kTextureSet,
                                     draw.textureSet, {});
    commandBuffer.draw(kQuadVertexCount, draw.instanceCount, 0, draw.firstInstance);
}

memory::Buffer& ParticlePass::reserve(std::size_t frameIndex, std::size_t count) {
    std::optional<memory::Buffer>& buffer = _buffers.at(frameIndex);
    const vk::DeviceSize needed = count * sizeof(plugins::backend::graphics::ParticleInstance);
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < count) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * sizeof(plugins::backend::graphics::ParticleInstance),
                       vk::BufferUsageFlagBits::eVertexBuffer, memory::Allocator::MemoryAccess::kStaging);
    }
    return *buffer;
}

}  // namespace rtype::render::vulkan::rendering
