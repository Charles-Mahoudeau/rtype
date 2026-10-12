/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorSetLayout
*/

#include "DescriptorSetLayout.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(std::uint32_t binding, vk::DescriptorType type,
                                                                       vk::ShaderStageFlags stages,
                                                                       std::uint32_t count) {
    if (count == 0) {
        throw std::invalid_argument(std::format("Descriptor binding {} has no descriptor", binding));
    }
    if (std::ranges::any_of(_bindings, [binding](const vk::DescriptorSetLayoutBinding& existing) {
            return existing.binding == binding;
        })) {
        throw std::invalid_argument(std::format("Descriptor binding {} is added twice", binding));
    }
    _bindings.emplace_back(binding, type, count, stages);
    return *this;
}

DescriptorSetLayout DescriptorSetLayout::Builder::build(const core::Device& device) const {
    return DescriptorSetLayout{
        vk::raii::DescriptorSetLayout{device.getDevice(), vk::DescriptorSetLayoutCreateInfo{}.setBindings(_bindings)},
        _bindings};
}

DescriptorSetLayout::DescriptorSetLayout(vk::raii::DescriptorSetLayout layout,
                                         std::vector<vk::DescriptorSetLayoutBinding> bindings)
    : _layout{std::move(layout)}, _bindings{std::move(bindings)} {}

}  // namespace rtype::render::vulkan::descriptor
