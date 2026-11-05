/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePass
*/

#include "ShapePass.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "MaterialBinding.hpp"
#include "ShaderLayout.hpp"
#include "ShapeBatch.hpp"
#include "ShapeInstance.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Shader.hpp"
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

/// @brief Binding of the instance values of a game material's shapes, after the ShapeInstance's.
constexpr std::uint32_t kValuesBinding = 1;
/// @brief First location of the instance values: four vec4, after the ShapeInstance's eight attributes.
constexpr std::uint32_t kValuesLocation = 8;
/// @brief Bytes of one instance's values: four vec4.
constexpr std::size_t kValuesSize = sizeof(plugins::backend::graphics::ShaderInstanceValues);

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
      _buffers(framesInFlight),
      _values(framesInFlight) {}

pipeline::GraphicsPipeline ShapePass::createMaterialPipeline(
    const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
    bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
    const vk::raii::PipelineCache* cache) {
    std::vector<vk::VertexInputBindingDescription> bindings{ShapeInstance::getBindings().front()};
    std::vector<vk::VertexInputAttributeDescription> attributes;
    for (const vk::VertexInputAttributeDescription& attribute : ShapeInstance::getAttributes()) {
        attributes.push_back(attribute);
    }
    if (instanceValues) {
        bindings.emplace_back(kValuesBinding, static_cast<std::uint32_t>(kValuesSize), vk::VertexInputRate::eInstance);
        for (std::uint32_t vector = 0; vector < plugins::backend::graphics::ShaderInstanceValues{}.size(); ++vector) {
            attributes.emplace_back(kValuesLocation + vector, kValuesBinding, vk::Format::eR32G32B32A32Sfloat,
                                    vector * static_cast<std::uint32_t>(sizeof(glm::vec4)));
        }
    }
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(vertex, fragment)
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setLayout(layout)
        .build(device, cache);
}

void ShapePass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const ShapeBatch& shapes,
                       const plugins::backend::graphics::Viewport& viewport, vk::DescriptorSet textureSet,
                       const MaterialFinder& findMaterial) {
    const std::span<const ShapeInstance> instances = shapes.getInstances();
    if (instances.empty() || viewport.size.x <= 0.0F || viewport.size.y <= 0.0F) {
        return;
    }
    memory::Buffer& buffer = reserve(_buffers.at(frameIndex), instances.size(), sizeof(ShapeInstance));
    buffer.write(std::as_bytes(instances));
    const std::span<const plugins::backend::graphics::ShaderInstanceValues> instanceValues = shapes.getInstanceValues();
    if (!instanceValues.empty()) {
        reserve(_values.at(frameIndex), instanceValues.size(), kValuesSize).write(std::as_bytes(instanceValues));
    }

    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(
        0, vk::Viewport{viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y, 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{static_cast<std::int32_t>(viewport.position.x),
                                                        static_cast<std::int32_t>(viewport.position.y)},
                                           vk::Extent2D{static_cast<std::uint32_t>(viewport.size.x),
                                                        static_cast<std::uint32_t>(viewport.size.y)}});
    const std::optional<memory::Buffer>& values = _values.at(frameIndex);
    const pipeline::GraphicsPipeline* bound = &_pipeline;
    bool instancesFromStart = false;
    for (const ShapeBatch::Batch& range : shapes.getBatches()) {
        MaterialBinding material = range.material != plugins::backend::graphics::MaterialId{} && textureSet
                                       ? findMaterial(range.material)
                                       : MaterialBinding{};
        if (material.instanceValues && !values) {
            material = MaterialBinding{};
        }
        const pipeline::GraphicsPipeline& drawn = material.pipeline != nullptr ? *material.pipeline : _pipeline;
        if (bound != &drawn) {
            drawn.bind(commandBuffer);
            bound = &drawn;
        }
        if (material.pipeline != nullptr) {
            const vk::PipelineLayout layout = *drawn.getLayout().getLayout();
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet, textureSet, {});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMaterialSet, material.set, {});
        }
        if (!material.instanceValues || !values) {
            if (!instancesFromStart) {
                commandBuffer.bindVertexBuffers(0, *buffer.getBuffer(), vk::DeviceSize{0});
                instancesFromStart = true;
            }
            commandBuffer.draw(kQuadVertexCount, range.instanceCount, 0, range.firstInstance);
            continue;
        }
        const std::array vertexBuffers{*buffer.getBuffer(), *values->getBuffer()};
        const std::array offsets{vk::DeviceSize{range.firstInstance} * sizeof(ShapeInstance),
                                 vk::DeviceSize{range.firstValues} * kValuesSize};
        commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
        instancesFromStart = false;
        commandBuffer.draw(kQuadVertexCount, range.instanceCount, 0, 0);
    }
}

memory::Buffer& ShapePass::reserve(std::optional<memory::Buffer>& buffer, std::size_t count, std::size_t elementSize) {
    const vk::DeviceSize needed = count * elementSize;
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < count) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * elementSize, vk::BufferUsageFlagBits::eVertexBuffer,
                       memory::Allocator::MemoryAccess::kStaging);
    }
    return *buffer;
}

}  // namespace rtype::render::vulkan::rendering
