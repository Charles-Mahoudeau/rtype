/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorWriter
*/

#pragma once

#include <cstdint>
#include <deque>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

/// @brief Collects what the bindings of a descriptor set point to, then writes them into a set in one
/// vkUpdateDescriptorSets call:
/// @code
/// DescriptorWriter{}
///     .writeBuffer(0, *uniformBuffer, 0, sizeof(Uniforms))
///     .writeImage(1, *view, *sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
///     .update(device, *set);
/// @endcode
class RTYPE_RENDER_VULKAN_API DescriptorWriter {
  public:
    DescriptorWriter() = default;
    ~DescriptorWriter() = default;
    DescriptorWriter(const DescriptorWriter&) = default;
    DescriptorWriter& operator=(const DescriptorWriter&) = default;
    DescriptorWriter(DescriptorWriter&&) noexcept = default;
    DescriptorWriter& operator=(DescriptorWriter&&) noexcept = default;

    /// @brief Points binding @p binding to @p view, read through @p sampler in @p layout.
    /// @param type eCombinedImageSampler, eSampledImage (no sampler) or eStorageImage (no sampler, eGeneral).
    DescriptorWriter& writeImage(std::uint32_t binding, vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout,
                                 vk::DescriptorType type = vk::DescriptorType::eCombinedImageSampler);

    /// @brief Points binding @p binding to @p range bytes of @p buffer from @p offset.
    /// @param type eUniformBuffer or eStorageBuffer (or their dynamic variants).
    DescriptorWriter& writeBuffer(std::uint32_t binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range,
                                  vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);

    /// @return The writes collected so far, aimed at @p set. They point into this writer: keep it alive while they
    /// are used.
    [[nodiscard]] std::vector<vk::WriteDescriptorSet> getWrites(vk::DescriptorSet set) const;

    /// @brief Writes everything collected into @p set. The set must not be in use by the GPU.
    void update(const core::Device& device, vk::DescriptorSet set) const;

    /// @brief Forgets everything collected, to reuse the writer for another set.
    void clear() noexcept;

  private:
    /// @brief One binding to write, pointing to its info in _images or _buffers.
    struct Write {
        std::uint32_t binding;    ///< Binding in the set.
        vk::DescriptorType type;  ///< Type of the descriptor.
        bool isImage;             ///< Whether the info is in _images (else in _buffers).
        std::size_t infoIndex;    ///< Index of the info.
    };

    std::vector<Write> _writes;                     ///< In the order they were added.
    std::deque<vk::DescriptorImageInfo> _images;    ///< Image infos; a deque keeps them in place as it grows.
    std::deque<vk::DescriptorBufferInfo> _buffers;  ///< Buffer infos; a deque keeps them in place as it grows.
};

}  // namespace rtype::render::vulkan::descriptor
