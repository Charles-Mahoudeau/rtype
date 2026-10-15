/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineCache
*/

#include "PipelineCache.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

PipelineCache::PipelineCache(const core::Device& device, std::span<const std::byte> initialData)
    : _cache{device.getDevice(),
             vk::PipelineCacheCreateInfo{}.setInitialDataSize(initialData.size()).setPInitialData(initialData.data())} {
}

std::vector<std::uint8_t> PipelineCache::getData() const { return _cache.getData(); }

}  // namespace rtype::render::vulkan::pipeline
