/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorAllocator
*/

#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

/// @brief Allocates descriptor sets without a limit known in advance: when its pool is full, it adds a new one.
///
/// @details For sets whose number grows with the game, such as one per texture. Every pool gets the same capacity,
/// given at construction; a set is allocated from the newest pool, and a new pool is created when it runs out of sets
/// or descriptors. Sets give their descriptors back to their pool when destroyed (DescriptorPool frees them).
/// @code
/// DescriptorAllocator allocator{device, 64, std::array{vk::DescriptorPoolSize{eCombinedImageSampler, 64}}};
/// vk::raii::DescriptorSet set = allocator.allocate(layout);
/// @endcode
/// @warning Destroy every set it allocated before it, and it before the Device.
class RTYPE_RENDER_VULKAN_API DescriptorAllocator {
  public:
    /// @param device Creates the pools and the sets; must outlive the allocator.
    /// @param setsPerPool Most sets of one pool.
    /// @param sizesPerPool Most descriptors of one pool, by type: enough for setsPerPool sets of the layouts used.
    /// @throws vk::SystemError If the first pool cannot be created.
    DescriptorAllocator(const core::Device& device, std::uint32_t setsPerPool,
                        std::span<const vk::DescriptorPoolSize> sizesPerPool);
    ~DescriptorAllocator() = default;
    DescriptorAllocator(const DescriptorAllocator&) = delete;
    DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;
    DescriptorAllocator(DescriptorAllocator&&) = delete;
    DescriptorAllocator& operator=(DescriptorAllocator&&) = delete;

    /// @return The number of pools created so far.
    [[nodiscard]] std::size_t getPoolCount() const noexcept { return _pools.size(); }

    /// @return A new set of @p layout, from a new pool if the newest one is full.
    /// @throws vk::SystemError If a new pool cannot be created, or the set does not fit in an empty pool (its layout
    /// needs more descriptors than sizesPerPool).
    [[nodiscard]] vk::raii::DescriptorSet allocate(const DescriptorSetLayout& layout);

  private:
    /// @brief Adds an empty pool.
    void addPool();

    const core::Device* _device;                 ///< Creates the pools; not owned.
    std::uint32_t _setsPerPool;                  ///< Capacity of each pool, in sets.
    std::vector<vk::DescriptorPoolSize> _sizes;  ///< Capacity of each pool, in descriptors by type.
    std::vector<DescriptorPool> _pools;          ///< Every pool created, the newest last.
};

}  // namespace rtype::render::vulkan::descriptor
