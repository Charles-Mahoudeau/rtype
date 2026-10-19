/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Image
*/

#include "Image.hpp"

#include <stdexcept>
#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::memory {

namespace {

/// @return A 2D image of @p extent, after checking it.
/// @throws std::invalid_argument If a side of @p extent is 0, which Vulkan does not allow.
vma::raii::Image createImage(const Allocator& allocator, vk::Extent2D extent, vk::Format format,
                             vk::ImageUsageFlags usage) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("Image extent must not be 0");
    }
    return allocator.createImage(vk::ImageCreateInfo{}
                                     .setImageType(vk::ImageType::e2D)
                                     .setFormat(format)
                                     .setExtent(vk::Extent3D{extent, 1})
                                     .setMipLevels(1)
                                     .setArrayLayers(1)
                                     .setSamples(vk::SampleCountFlagBits::e1)
                                     .setTiling(vk::ImageTiling::eOptimal)
                                     .setUsage(usage)
                                     .setSharingMode(vk::SharingMode::eExclusive)
                                     .setInitialLayout(vk::ImageLayout::eUndefined));
}

/// @return A view of every texel of @p image, color aspect.
vk::raii::ImageView createView(const core::Device& device, vk::Image image, vk::Format format) {
    return vk::raii::ImageView{device.getDevice(), vk::ImageViewCreateInfo{}
                                                       .setImage(image)
                                                       .setViewType(vk::ImageViewType::e2D)
                                                       .setFormat(format)
                                                       .setSubresourceRange(vk::ImageSubresourceRange{
                                                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})};
}

}  // namespace

Image::Image(const core::Device& device, const Allocator& allocator, vk::Extent2D extent, vk::Format format,
             vk::ImageUsageFlags usage)
    : _image{createImage(allocator, extent, format, usage)},
      _view{createView(device, *_image, format)},
      _extent{extent},
      _format{format} {}

}  // namespace rtype::render::vulkan::memory
