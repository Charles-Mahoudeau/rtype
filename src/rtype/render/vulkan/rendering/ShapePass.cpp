/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePass
*/

#include "ShapePass.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "ShapeInstance.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "shape.frag.h"
#include "shape.vert.h"

namespace rtype::render::vulkan::rendering {

namespace {

/// @brief Vertices of the quad: two triangles, built by shape.vert from gl_VertexIndex.
constexpr std::uint32_t kQuadVertexCount = 6;

/// @return The shape pipeline.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          const vk::raii::PipelineCache* cache) {
    const auto bindings = ShapeInstance::getBindings();
    const auto attributes = ShapeInstance::getAttributes();
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("shape.vert", rtype_shader_shape_vert),
                    shaders.get("shape.frag", rtype_shader_shape_frag))
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setLayout(layout)
        .build(device, cache);
}

}  // namespace

ShapePass::ShapePass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                     const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                     const vk::raii::PipelineCache* cache, std::size_t framesInFlight)
    : _allocator{&allocator},
      _pipeline{createPipeline(device, shaders, layout, colorFormat, cache)},
      _buffers(framesInFlight) {}

void ShapePass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                       std::span<const ShapeInstance> shapes, const plugins::backend::graphics::Viewport& viewport) {
    if (shapes.empty() || viewport.size.x <= 0.0F || viewport.size.y <= 0.0F) {
        return;
    }
    memory::Buffer& buffer = reserve(frameIndex, shapes.size());
    buffer.write(std::as_bytes(shapes));

    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(
        0, vk::Viewport{viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y, 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{static_cast<std::int32_t>(viewport.position.x),
                                                        static_cast<std::int32_t>(viewport.position.y)},
                                           vk::Extent2D{static_cast<std::uint32_t>(viewport.size.x),
                                                        static_cast<std::uint32_t>(viewport.size.y)}});
    commandBuffer.bindVertexBuffers(0, *buffer.getBuffer(), vk::DeviceSize{0});
    commandBuffer.draw(kQuadVertexCount, static_cast<std::uint32_t>(shapes.size()), 0, 0);
}

memory::Buffer& ShapePass::reserve(std::size_t frameIndex, std::size_t count) {
    std::optional<memory::Buffer>& buffer = _buffers.at(frameIndex);
    const vk::DeviceSize needed = count * sizeof(ShapeInstance);
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < count) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * sizeof(ShapeInstance), vk::BufferUsageFlagBits::eVertexBuffer,
                       memory::Allocator::MemoryAccess::kStaging);
    }
    return *buffer;
}

}  // namespace rtype::render::vulkan::rendering
