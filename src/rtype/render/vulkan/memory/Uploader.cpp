/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Uploader
*/

#include "Uploader.hpp"

#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/rendering/DynamicRendering.hpp"

namespace rtype::render::vulkan::memory {

namespace {

/// @brief Stages that may use an uploaded buffer after its copy: vertex and index fetching, the shaders, and another
/// copy into it (two uploads to the same buffer in one batch must not run in any order).
constexpr vk::PipelineStageFlags2 kAfterCopyStages =
    vk::PipelineStageFlagBits2::eVertexAttributeInput | vk::PipelineStageFlagBits2::eIndexInput |
    vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eFragmentShader |
    vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eCopy;

/// @brief How those stages may use it: read as vertices, indices, uniforms or storage, or written by a copy.
constexpr vk::AccessFlags2 kAfterCopyAccesses =
    vk::AccessFlagBits2::eVertexAttributeRead | vk::AccessFlagBits2::eIndexRead | vk::AccessFlagBits2::eUniformRead |
    vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eTransferWrite;

/// @brief Before an image's copy: its contents are discarded (UNDEFINED), and the copy writes it.
constexpr rendering::ImageTransition kToTransferDst{
    .oldLayout = vk::ImageLayout::eUndefined,
    .newLayout = vk::ImageLayout::eTransferDstOptimal,
    .srcStage = vk::PipelineStageFlagBits2::eNone,
    .srcAccess = vk::AccessFlagBits2::eNone,
    .dstStage = vk::PipelineStageFlagBits2::eCopy,
    .dstAccess = vk::AccessFlagBits2::eTransferWrite,
};

/// @brief After an image's copy: the fragment and compute shaders of every later submission sample it.
constexpr rendering::ImageTransition kTransferDstToShaderRead{
    .oldLayout = vk::ImageLayout::eTransferDstOptimal,
    .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .srcStage = vk::PipelineStageFlagBits2::eCopy,
    .srcAccess = vk::AccessFlagBits2::eTransferWrite,
    .dstStage = vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader,
    .dstAccess = vk::AccessFlagBits2::eShaderSampledRead,
};

}  // namespace

Uploader::Uploader(const core::Device& device, const Allocator& allocator)
    : _device{&device},
      _allocator{&allocator},
      _commandPool{device, device.getGraphicsQueue().family},
      _commandBuffer{_commandPool.allocate()},
      _fence{device} {}

Uploader::~Uploader() {
    try {
        flush();
    } catch (const std::exception& error) {
        std::cerr << "[Uploader] Pending uploads failed at destruction: " << error.what() << '\n';
    }
}

void Uploader::upload(std::span<const std::byte> data, Buffer& destination, vk::DeviceSize offset) {
    if (data.empty()) {
        return;
    }
    if (offset > destination.getSize() || data.size() > destination.getSize() - offset) {
        throw std::out_of_range(
            std::format("Uploader: upload out of range: {} + {} > {}", offset, data.size(), destination.getSize()));
    }
    if (destination.isHostVisible()) {
        destination.write(data, offset);
        return;
    }

    Buffer& staging = _stagingBuffers.emplace_back(*_allocator, data.size(), vk::BufferUsageFlagBits::eTransferSrc,
                                                   Allocator::MemoryAccess::kStaging);
    staging.write(data);

    beginRecording();
    const vk::raii::CommandBuffer& commandBuffer = _commandBuffer.getCommandBuffer();
    commandBuffer.copyBuffer(*staging.getBuffer(), *destination.getBuffer(),
                             vk::BufferCopy{}.setSrcOffset(0).setDstOffset(offset).setSize(data.size()));
    const auto barrier = vk::BufferMemoryBarrier2{}
                             .setSrcStageMask(vk::PipelineStageFlagBits2::eCopy)
                             .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                             .setDstStageMask(kAfterCopyStages)
                             .setDstAccessMask(kAfterCopyAccesses)
                             .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                             .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                             .setBuffer(*destination.getBuffer())
                             .setOffset(offset)
                             .setSize(data.size());
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{}.setBufferMemoryBarriers(barrier));
}

void Uploader::upload(std::span<const std::byte> texels, Image& destination) {
    const vk::Extent2D extent = destination.getExtent();
    const vk::DeviceSize size = vk::DeviceSize{extent.width} * extent.height * vk::blockSize(destination.getFormat());
    if (texels.size() != size) {
        throw std::invalid_argument(std::format("Uploader: {} bytes for a {}x{} {} image of {} bytes", texels.size(),
                                                extent.width, extent.height, vk::to_string(destination.getFormat()),
                                                size));
    }

    Buffer& staging = _stagingBuffers.emplace_back(*_allocator, size, vk::BufferUsageFlagBits::eTransferSrc,
                                                   Allocator::MemoryAccess::kStaging);
    staging.write(texels);

    beginRecording();
    const vk::raii::CommandBuffer& commandBuffer = _commandBuffer.getCommandBuffer();
    const vk::Image image = *destination.getImage();
    rendering::transitionImage(commandBuffer, image, kToTransferDst);
    commandBuffer.copyBufferToImage(
        *staging.getBuffer(), image, vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy{}
            .setImageSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1})
            .setImageExtent(vk::Extent3D{extent, 1}));
    rendering::transitionImage(commandBuffer, image, kTransferDstToShaderRead);
}

void Uploader::flush() {
    if (!_recording) {
        return;
    }
    _commandBuffer.end();
    _recording = false;

    const vk::CommandBufferSubmitInfo commandBufferInfo{*_commandBuffer.getCommandBuffer()};
    _device->getGraphicsQueue().handle.submit2(vk::SubmitInfo2{}.setCommandBufferInfos(commandBufferInfo),
                                               *_fence.getFence());
    _fence.wait();
    _fence.reset();
    _stagingBuffers.clear();
    _commandPool.reset();
}

void Uploader::beginRecording() {
    if (!_recording) {
        _commandBuffer.begin();
        _recording = true;
    }
}

}  // namespace rtype::render::vulkan::memory
