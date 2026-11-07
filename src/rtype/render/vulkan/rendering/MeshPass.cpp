/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MeshPass
*/

#include "MeshPass.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <string_view>
#include <vulkan/vulkan_raii.hpp>

#include "ShaderLayout.hpp"
#include "mesh.frag.h"
#include "mesh.vert.h"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "render/vulkan/pipeline/Vertex2D.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"

namespace rtype::render::vulkan::rendering {

namespace {

/// @brief Elements a buffer holds when it is first made: it doubles from there.
constexpr std::size_t kInitialCapacity = 4096;

static_assert(sizeof(contract::renderer::MeshVertex) == sizeof(pipeline::Vertex2D) &&
                  offsetof(contract::renderer::MeshVertex, uv) == offsetof(pipeline::Vertex2D, uv) &&
                  offsetof(contract::renderer::MeshVertex, color) == offsetof(pipeline::Vertex2D, color),
              "A MeshVertex is read as a Vertex2D by the mesh shaders");

/// @return The mesh pipeline.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          const vk::raii::PipelineCache* cache) {
    const auto bindings = pipeline::Vertex2D::getBindings();
    const auto attributes = pipeline::Vertex2D::getAttributes();
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("mesh.vert", rtype_shader_mesh_vert), shaders.get("mesh.frag", rtype_shader_mesh_frag))
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setLayout(layout)
        .build(device, cache);
}

/// @return The scissor of a clip rectangle, clamped to the framebuffer; an empty one when nothing of it is inside.
vk::Rect2D toScissor(glm::vec2 clipMin, glm::vec2 clipMax, glm::uvec2 framebufferSize) {
    const glm::vec2 size{framebufferSize};
    const glm::vec2 low = glm::clamp(clipMin, glm::vec2{0.0F}, size);
    const glm::vec2 high = glm::clamp(clipMax, glm::vec2{0.0F}, size);
    const glm::vec2 extent = glm::max(high - low, glm::vec2{0.0F});
    return vk::Rect2D{vk::Offset2D{static_cast<std::int32_t>(low.x), static_cast<std::int32_t>(low.y)},
                      vk::Extent2D{static_cast<std::uint32_t>(extent.x), static_cast<std::uint32_t>(extent.y)}};
}

}  // namespace

MeshPass::MeshPass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                   const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache,
                   std::size_t framesInFlight)
    : _allocator{&allocator},
      _pipeline{createPipeline(device, shaders, layout, colorFormat, cache)},
      _vertices(framesInFlight),
      _indices(framesInFlight) {
    device.setName(*_pipeline.getPipeline(), "Mesh pipeline");
}

void MeshPass::upload(std::size_t frameIndex, const contract::renderer::Mesh& mesh) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return;
    }
    reserve(_vertices.at(frameIndex), mesh.vertices.size(), sizeof(contract::renderer::MeshVertex),
            vk::BufferUsageFlagBits::eVertexBuffer, "Mesh vertices")
        .write(std::as_bytes(std::span{mesh.vertices}));
    reserve(_indices.at(frameIndex), mesh.indices.size(), sizeof(std::uint32_t), vk::BufferUsageFlagBits::eIndexBuffer,
            "Mesh indices")
        .write(std::as_bytes(std::span{mesh.indices}));
}

void MeshPass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                      const contract::renderer::Mesh& mesh, glm::uvec2 framebufferSize, const SetFinder& findSet,
                      vk::DescriptorSet whiteSet) {
    const std::optional<memory::Buffer>& vertices = _vertices.at(frameIndex);
    const std::optional<memory::Buffer>& indices = _indices.at(frameIndex);
    if (mesh.commands.empty() || !vertices || !indices || framebufferSize.x == 0 || framebufferSize.y == 0) {
        return;
    }
    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(0, vk::Viewport{0.0F, 0.0F, static_cast<float>(framebufferSize.x),
                                              static_cast<float>(framebufferSize.y), 0.0F, 1.0F});
    commandBuffer.bindVertexBuffers(0, *vertices->getBuffer(), vk::DeviceSize{0});
    commandBuffer.bindIndexBuffer(*indices->getBuffer(), 0, vk::IndexType::eUint32);
    const vk::PipelineLayout layout = *_pipeline.getLayout().getLayout();
    vk::DescriptorSet bound;
    for (const contract::renderer::MeshCommand& command : mesh.commands) {
        const vk::Rect2D scissor = toScissor(command.clipMin, command.clipMax, framebufferSize);
        if (scissor.extent.width == 0 || scissor.extent.height == 0 || command.indexCount == 0) {
            continue;
        }
        vk::DescriptorSet set =
            command.texture != contract::renderer::TextureId{} ? findSet(command.texture) : vk::DescriptorSet{};
        if (!set) {
            set = whiteSet;
        }
        if (set != bound) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet, set, {});
            bound = set;
        }
        commandBuffer.setScissor(0, scissor);
        commandBuffer.drawIndexed(command.indexCount, 1, command.indexOffset, 0, 0);
    }
}

memory::Buffer& MeshPass::reserve(std::optional<memory::Buffer>& buffer, std::size_t count, std::size_t elementSize,
                                  vk::BufferUsageFlagBits usage, std::string_view name) {
    const vk::DeviceSize needed = count * elementSize;
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < count) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * elementSize, usage, memory::Allocator::MemoryAccess::kStaging);
        _allocator->getDevice().setName(*buffer->getBuffer(), name);
    }
    return *buffer;
}

}  // namespace rtype::render::vulkan::rendering
