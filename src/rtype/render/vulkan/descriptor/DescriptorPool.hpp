/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorPool
*/

#pragma once

#include <cstdint>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "DescriptorSetLayout.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

/// @brief Allocates descriptor sets.
///
/// @details Created with VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT: each set is a vk::raii::DescriptorSet
/// that gives its descriptors back to the pool when destroyed.
/// @warning Destroy every set it allocated before it, and it before the Device.
class RTYPE_RENDER_VULKAN_API DescriptorPool {
  public:
    /// @param device The device the pool and its sets are created on; must outlive the pool.
    /// @param maxSets Most sets allocated at once.
    /// @param sizes Most descriptors allocated at once, by type, all sets together.
    /// @throws vk::SystemError If the pool cannot be created.
    DescriptorPool(const core::Device& device, std::uint32_t maxSets, std::span<const vk::DescriptorPoolSize> sizes);
    ~DescriptorPool() = default;
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&&) noexcept = default;
    DescriptorPool& operator=(DescriptorPool&&) noexcept = default;

    /// @return The Vulkan pool.
    [[nodiscard]] const vk::raii::DescriptorPool& getPool() const noexcept { return _pool; }

    /// @return A new set of @p layout, to fill with a DescriptorWriter.
    /// @throws vk::SystemError If the pool is out of sets or descriptors.
    [[nodiscard]] vk::raii::DescriptorSet allocate(const DescriptorSetLayout& layout);

  private:
    const core::Device* _device;     ///< Allocates the sets; not owned.
    vk::raii::DescriptorPool _pool;  ///< Destroyed on destruction.
};

}  // namespace rtype::render::vulkan::descriptor
