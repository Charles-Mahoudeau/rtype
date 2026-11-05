/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePass
*/

#include "SpritePass.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "MaterialBinding.hpp"
#include "ShaderLayout.hpp"
#include "SpriteBatch.hpp"
#include "glyph.frag.h"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Shader.hpp"
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

/// @brief Binding of the instance values of a game material's sprites, after the SpriteInstance's.
constexpr std::uint32_t kValuesBinding = 1;
/// @brief First location of the instance values: four vec4, after the SpriteInstance's seven attributes.
constexpr std::uint32_t kValuesLocation = 7;
/// @brief Bytes of one instance's values: four vec4.
constexpr std::size_t kValuesSize = sizeof(plugins::backend::graphics::ShaderInstanceValues);

/// @return A pipeline of the default shader: sprite.vert and @p fragment (sprite.frag or glyph.frag).
pipeline::GraphicsPipeline createPipeline(const core::Device& device, const vk::raii::ShaderModule& fragment,
                                          pipeline::ShaderCache& shaders, const pipeline::PipelineLayout& layout,
                                          vk::Format colorFormat, const vk::raii::PipelineCache* cache) {
    const auto bindings = SpriteInstance::getBindings();
    const auto attributes = SpriteInstance::getAttributes();
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("sprite.vert", rtype_shader_sprite_vert), fragment)
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
      _pipeline{createPipeline(device, shaders.get("sprite.frag", rtype_shader_sprite_frag), shaders, layout,
                               colorFormat, cache)},
      _glyphPipeline{createPipeline(device, shaders.get("glyph.frag", rtype_shader_glyph_frag), shaders, layout,
                                    colorFormat, cache)},
      _buffers(framesInFlight),
      _values(framesInFlight) {
    device.setName(*_pipeline.getPipeline(), "Sprite pipeline");
    device.setName(*_glyphPipeline.getPipeline(), "Glyph pipeline");
}

pipeline::GraphicsPipeline SpritePass::createMaterialPipeline(
    const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
    bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
    const vk::raii::PipelineCache* cache) {
    std::vector<vk::VertexInputBindingDescription> bindings{SpriteInstance::getBindings().front()};
    std::vector<vk::VertexInputAttributeDescription> attributes;
    for (const vk::VertexInputAttributeDescription& attribute : SpriteInstance::getAttributes()) {
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

void SpritePass::upload(std::size_t frameIndex, const SpriteBatch& batch) {
    const std::span<const SpriteInstance> instances = batch.getInstances();
    if (instances.empty()) {
        return;
    }
    reserve(_buffers.at(frameIndex), instances.size(), sizeof(SpriteInstance), "Sprite instances")
        .write(std::as_bytes(instances));
    const std::span<const plugins::backend::graphics::ShaderInstanceValues> values = batch.getInstanceValues();
    if (!values.empty()) {
        reserve(_values.at(frameIndex), values.size(), kValuesSize, "Sprite instance values")
            .write(std::as_bytes(values));
    }
}

void SpritePass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                        std::span<const SpriteBatch::Batch> batches,
                        const plugins::backend::graphics::Viewport& viewport, const TextureSetFinder& findTextureSet,
                        const MaterialFinder& findMaterial) {
    const std::optional<memory::Buffer>& buffer = _buffers.at(frameIndex);
    if (batches.empty() || !buffer || viewport.size.x <= 0.0F || viewport.size.y <= 0.0F) {
        return;
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
    for (const SpriteBatch::Batch& range : batches) {
        const vk::DescriptorSet set = findTextureSet(range.texture);
        if (!set) {
            continue;
        }
        MaterialBinding material = range.material != plugins::backend::graphics::MaterialId{}
                                       ? findMaterial(range.material)
                                       : MaterialBinding{};
        if (material.instanceValues && !values) {
            material = MaterialBinding{};
        }
        const pipeline::GraphicsPipeline& fallback =
            range.target == plugins::backend::graphics::ShaderTarget::kGlyph ? _glyphPipeline : _pipeline;
        const pipeline::GraphicsPipeline& drawn = material.pipeline != nullptr ? *material.pipeline : fallback;
        if (bound != &drawn) {
            drawn.bind(commandBuffer);
            bound = &drawn;
        }
        const vk::PipelineLayout layout = *drawn.getLayout().getLayout();
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet, set, {});
        if (material.pipeline != nullptr) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMaterialSet, material.set, {});
        }
        if (!material.instanceValues || !values) {
            if (!instancesFromStart) {
                commandBuffer.bindVertexBuffers(0, *buffer->getBuffer(), vk::DeviceSize{0});
                instancesFromStart = true;
            }
            commandBuffer.draw(kQuadVertexCount, range.instanceCount, 0, range.firstInstance);
            continue;
        }
        const std::array vertexBuffers{*buffer->getBuffer(), *values->getBuffer()};
        const std::array offsets{vk::DeviceSize{range.firstInstance} * sizeof(SpriteInstance),
                                 vk::DeviceSize{range.firstValues} * kValuesSize};
        commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
        instancesFromStart = false;
        commandBuffer.draw(kQuadVertexCount, range.instanceCount, 0, 0);
    }
}

memory::Buffer& SpritePass::reserve(std::optional<memory::Buffer>& buffer, std::size_t count, std::size_t elementSize,
                                    std::string_view name) {
    const vk::DeviceSize needed = count * elementSize;
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < count) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * elementSize, vk::BufferUsageFlagBits::eVertexBuffer,
                       memory::Allocator::MemoryAccess::kStaging);
        _allocator->getDevice().setName(*buffer->getBuffer(), name);
    }
    return *buffer;
}

}  // namespace rtype::render::vulkan::rendering
