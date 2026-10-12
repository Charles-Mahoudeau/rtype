/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorWriter
*/

#include "DescriptorWriter.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

DescriptorWriter& DescriptorWriter::writeImage(std::uint32_t binding, vk::ImageView view, vk::Sampler sampler,
                                               vk::ImageLayout layout, vk::DescriptorType type) {
    _images.emplace_back(sampler, view, layout);
    _writes.push_back(Write{.binding = binding, .type = type, .isImage = true, .infoIndex = _images.size() - 1});
    return *this;
}

DescriptorWriter& DescriptorWriter::writeBuffer(std::uint32_t binding, vk::Buffer buffer, vk::DeviceSize offset,
                                                vk::DeviceSize range, vk::DescriptorType type) {
    _buffers.emplace_back(buffer, offset, range);
    _writes.push_back(Write{.binding = binding, .type = type, .isImage = false, .infoIndex = _buffers.size() - 1});
    return *this;
}

std::vector<vk::WriteDescriptorSet> DescriptorWriter::getWrites(vk::DescriptorSet set) const {
    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(_writes.size());
    for (const Write& write : _writes) {
        auto descriptor = vk::WriteDescriptorSet{}
                              .setDstSet(set)
                              .setDstBinding(write.binding)
                              .setDescriptorCount(1)
                              .setDescriptorType(write.type);
        if (write.isImage) {
            descriptor.setPImageInfo(&_images.at(write.infoIndex));
        } else {
            descriptor.setPBufferInfo(&_buffers.at(write.infoIndex));
        }
        writes.push_back(descriptor);
    }
    return writes;
}

void DescriptorWriter::update(const core::Device& device, vk::DescriptorSet set) const {
    device.getDevice().updateDescriptorSets(getWrites(set), {});
}

void DescriptorWriter::clear() noexcept {
    _writes.clear();
    _images.clear();
    _buffers.clear();
}

}  // namespace rtype::render::vulkan::descriptor
