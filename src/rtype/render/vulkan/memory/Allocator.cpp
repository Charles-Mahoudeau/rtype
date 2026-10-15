/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Allocator
*/

// The VMA implementation is compiled here, and only here. Apple Clang warns about missing nullability annotations
// inside it: not our code, silenced for this include only.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifndef NDEBUG
#include <iostream>
#endif
#include <string>
#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/core/Instance.hpp"
#include "render/vulkan/core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::memory {

namespace {

/// @return The VMA settings for a resource the CPU accesses as @p access.
vma::AllocationCreateInfo toAllocationInfo(Allocator::MemoryAccess access) {
    using Flag = vma::AllocationCreateFlagBits;
    using Memory = vk::MemoryPropertyFlagBits;
    vma::AllocationCreateInfo info{};
    info.setUsage(vma::MemoryUsage::eAuto);
    switch (access) {
        case Allocator::MemoryAccess::kGpuOnly:
            break;
        case Allocator::MemoryAccess::kStaging:
            info.setFlags(Flag::eHostAccessSequentialWrite | Flag::eMapped)
                .setRequiredFlags(Memory::eHostVisible | Memory::eHostCoherent);
            break;
        case Allocator::MemoryAccess::kUpload:
            info.setFlags(Flag::eHostAccessSequentialWrite | Flag::eHostAccessAllowTransferInstead | Flag::eMapped);
            break;
        case Allocator::MemoryAccess::kReadback:
            info.setFlags(Flag::eHostAccessRandom | Flag::eMapped).setPreferredFlags(Memory::eHostCached);
            break;
    }
    return info;
}

/// @return The VMA create info of the allocator: Vulkan 1.3, plus buffer device addresses when supported.
/// instance, device and pVulkanFunctions stay null: vma::raii::createAllocator fills them from the vk::raii objects.
vma::AllocatorCreateInfo makeAllocatorInfo(const core::PhysicalDevice& physicalDevice) {
    vma::AllocatorCreateInfo info{};
    info.setPhysicalDevice(*physicalDevice.getPhysicalDevice()).setVulkanApiVersion(vk::ApiVersion13);
    if (physicalDevice.getOptionalFeatures().bufferDeviceAddress) {
        info.setFlags(vma::AllocatorCreateFlagBits::eBufferDeviceAddress);
    }
    return info;
}

}  // namespace

Allocator::Allocator(const core::Instance& instance, const core::PhysicalDevice& physicalDevice,
                     const core::Device& device)
    : _allocator{
          vma::raii::createAllocator(instance.getInstance(), device.getDevice(), makeAllocatorInfo(physicalDevice))} {}

Allocator::~Allocator() {
#ifndef NDEBUG
    std::clog << "[VMA] Statistics before destroying the allocator:\n" << buildStatsString(false) << '\n';
#endif
}

vma::raii::Buffer Allocator::createBuffer(const vk::BufferCreateInfo& bufferInfo, MemoryAccess access) const {
    return _allocator.createBuffer(bufferInfo, toAllocationInfo(access));
}

vma::raii::Image Allocator::createImage(const vk::ImageCreateInfo& imageInfo, MemoryAccess access) const {
    return _allocator.createImage(imageInfo, toAllocationInfo(access));
}

bool Allocator::isHostVisible(const vma::raii::Allocation& allocation) {
    return static_cast<bool>(allocation.getMemoryProperties() & vk::MemoryPropertyFlagBits::eHostVisible);
}

std::string Allocator::buildStatsString(bool detailed) const {
    const vma::raii::StatsString stats = _allocator.buildStatsString(detailed ? VK_TRUE : VK_FALSE);
    return std::string{*stats};
}

}  // namespace rtype::render::vulkan::memory
