/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameGlobals
*/

#include "FrameGlobals.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorPool.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/rendering/FrameUniforms.hpp"
#include "render/vulkan/rendering/ShaderLayout.hpp"

namespace rtype::render::vulkan::frame {

namespace {

/// @return The layout of set 0: one uniform buffer, readable by every shader stage the renderer uses.
descriptor::DescriptorSetLayout createSetLayout(const core::Device& device) {
    return descriptor::DescriptorSetLayout::Builder{}
        .addBinding(
            rendering::kFrameUniformsBinding, vk::DescriptorType::eUniformBuffer,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
        .build(device);
}

/// @return A pool sized for exactly one set, holding one uniform buffer, per frame in flight.
descriptor::DescriptorPool createPool(const core::Device& device, std::size_t framesInFlight) {
    const auto frameCount = static_cast<std::uint32_t>(framesInFlight);
    const std::array sizes{vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, frameCount}};
    return descriptor::DescriptorPool{device, frameCount, sizes};
}

}  // namespace

FrameGlobals::FrameGlobals(const core::Device& device, const memory::Allocator& allocator, std::size_t framesInFlight)
    : _setLayout{createSetLayout(device)}, _pool{createPool(device, framesInFlight)} {
    _buffers.reserve(framesInFlight);
    _sets.reserve(framesInFlight);
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        const memory::Buffer& buffer =
            _buffers.emplace_back(allocator, sizeof(rendering::FrameUniforms), vk::BufferUsageFlagBits::eUniformBuffer,
                                  memory::Allocator::MemoryAccess::kStaging);
        const vk::raii::DescriptorSet& set = _sets.emplace_back(_pool.allocate(_setLayout));
        descriptor::DescriptorWriter{}
            .writeBuffer(rendering::kFrameUniformsBinding, *buffer.getBuffer(), 0, sizeof(rendering::FrameUniforms))
            .update(device, *set);
    }
}

void FrameGlobals::update(std::size_t frameIndex, const rendering::FrameUniforms& uniforms) {
    _buffers.at(frameIndex).write(std::as_bytes(std::span{&uniforms, 1}));
}

void FrameGlobals::bind(const vk::raii::CommandBuffer& commandBuffer, const pipeline::PipelineLayout& layout,
                        std::size_t frameIndex) const {
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *layout.getLayout(), rendering::kFrameSet,
                                     *_sets.at(frameIndex), {});
}

}  // namespace rtype::render::vulkan::frame
