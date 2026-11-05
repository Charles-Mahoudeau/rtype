/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorAllocator
*/

#include "DescriptorAllocator.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vulkan/vulkan_raii.hpp>

#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

DescriptorAllocator::DescriptorAllocator(const core::Device& device, std::uint32_t setsPerPool,
                                         std::span<const vk::DescriptorPoolSize> sizesPerPool, std::string_view name)
    : _device{&device}, _setsPerPool{setsPerPool}, _sizes{sizesPerPool.begin(), sizesPerPool.end()}, _name{name} {
    addPool();
}

vk::raii::DescriptorSet DescriptorAllocator::allocate(const DescriptorSetLayout& layout) {
    try {
        return _pools.back().allocate(layout);
    } catch (const vk::OutOfPoolMemoryError&) {
        addPool();
    } catch (const vk::FragmentedPoolError&) {
        addPool();
    }
    return _pools.back().allocate(layout);
}

void DescriptorAllocator::addPool() {
    const DescriptorPool& pool = _pools.emplace_back(*_device, _setsPerPool, _sizes);
    _device->setName(*pool.getPool(), _name);
}

}  // namespace rtype::render::vulkan::descriptor
