/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePass
*/

#include "SpritePass.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "ShaderLayout.hpp"
#include "SpriteBatch.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "sprite.frag.h"
#include "sprite.vert.h"

namespace rtype::render::vulkan::rendering {

namespace {

/// @brief Vertices of the unit quad: two triangles, built by sprite.vert from gl_VertexIndex.
constexpr std::uint32_t kQuadVertexCount = 6;

/// @return The sprite pipeline.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          const vk::raii::PipelineCache* cache) {
    const auto bindings = SpriteInstance::getBindings();
    const auto attributes = SpriteInstance::getAttributes();
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("sprite.vert", rtype_shader_sprite_vert),
                    shaders.get("sprite.frag", rtype_shader_sprite_frag))
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setLayout(layout)
        .build(device, cache);
}

}  // namespace

SpritePass::SpritePass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                       const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                       const vk::raii::PipelineCache* cache, std::size_t framesInFlight)
    : _allocator{&allocator},
      _layout{&layout},
      _pipeline{createPipeline(device, shaders, layout, colorFormat, cache)},
      _buffers(framesInFlight) {}

void SpritePass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const SpriteBatch& batch,
                        const plugins::backend::graphics::Viewport& viewport, const TextureSetFinder& findTextureSet) {
    const std::span<const SpriteInstance> instances = batch.getInstances();
    if (instances.empty() || viewport.size.x <= 0.0F || viewport.size.y <= 0.0F) {
        return;
    }
    memory::Buffer& buffer = reserve(frameIndex, instances.size());
    buffer.write(std::as_bytes(instances));

    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(
        0, vk::Viewport{viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y, 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{static_cast<std::int32_t>(viewport.position.x),
                                                        static_cast<std::int32_t>(viewport.position.y)},
                                           vk::Extent2D{static_cast<std::uint32_t>(viewport.size.x),
                                                        static_cast<std::uint32_t>(viewport.size.y)}});
    commandBuffer.bindVertexBuffers(0, *buffer.getBuffer(), vk::DeviceSize{0});
    for (const SpriteBatch::Batch& range : batch.getBatches()) {
        const vk::DescriptorSet set = findTextureSet(range.texture);
        if (!set) {
            continue;
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_layout->getLayout(), kTextureSet, set, {});
        commandBuffer.draw(kQuadVertexCount, range.instanceCount, 0, range.firstInstance);
    }
}

memory::Buffer& SpritePass::reserve(std::size_t frameIndex, std::size_t count) {
    std::optional<memory::Buffer>& buffer = _buffers.at(frameIndex);
    const vk::DeviceSize needed = count * sizeof(SpriteInstance);
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < count) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * sizeof(SpriteInstance), vk::BufferUsageFlagBits::eVertexBuffer,
                       memory::Allocator::MemoryAccess::kStaging);
    }
    return *buffer;
}

}  // namespace rtype::render::vulkan::rendering
