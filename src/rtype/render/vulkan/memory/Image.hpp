/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Image
*/

#pragma once

#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::memory {

/// @brief A 2D image, its memory and a view of it, allocated by the Allocator: textures, render targets...
///
/// @details One mip level, one layer, the color aspect, in device local memory (MemoryAccess::kGpuOnly). It starts in
/// the UNDEFINED layout: fill a texture with Uploader::upload(), which leaves it ready to be sampled.
/// @warning Destroy it before the Allocator and the Device, and only once the GPU is done with it (see
/// core::DeletionQueue).
class RTYPE_RENDER_VULKAN_API Image {
  public:
    /// @param device Creates the view; must outlive the image.
    /// @param allocator Allocates the image and its memory; must outlive the image.
    /// @param extent Its size, in texels; neither side 0.
    /// @param format Its texel format, e.g. eR8G8B8A8Srgb.
    /// @param usage How it is used: eSampled, eColorAttachment... plus eTransferDst to receive an upload.
    /// @throws std::invalid_argument If a side of @p extent is 0.
    /// @throws vk::SystemError If the image, its memory or its view cannot be created.
    Image(const core::Device& device, const Allocator& allocator, vk::Extent2D extent, vk::Format format,
          vk::ImageUsageFlags usage);
    ~Image() = default;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) noexcept = default;
    Image& operator=(Image&&) noexcept = default;

    /// @return The Vulkan image and its allocation; *getImage() is the vk::Image to copy into or transition.
    [[nodiscard]] const vma::raii::Image& getImage() const noexcept { return _image; }
    /// @return The view of the whole image, to bind in a descriptor or use as an attachment.
    [[nodiscard]] const vk::raii::ImageView& getImageView() const noexcept { return _view; }
    /// @return Its size, in texels.
    [[nodiscard]] vk::Extent2D getExtent() const noexcept { return _extent; }
    /// @return Its texel format.
    [[nodiscard]] vk::Format getFormat() const noexcept { return _format; }

  private:
    // The view is declared after the image, so it is destroyed before it.
    vma::raii::Image _image;    ///< The image and its memory, freed on destruction.
    vk::raii::ImageView _view;  ///< The view of the whole image.
    vk::Extent2D _extent;       ///< In texels.
    vk::Format _format;         ///< Texel format.
};

}  // namespace rtype::render::vulkan::memory
