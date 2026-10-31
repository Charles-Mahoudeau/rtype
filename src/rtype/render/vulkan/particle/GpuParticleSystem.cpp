/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GpuParticleSystem
*/

#include "GpuParticleSystem.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "GpuParticleData.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/memory/Uploader.hpp"

namespace rtype::render::vulkan::particle {

namespace {

/// @brief Vertices of a particle's quad, the indirect command's vertexCount.
constexpr std::uint32_t kQuadVertexCount = 6;

/// @return A device local buffer of @p size bytes.
memory::Buffer makeBuffer(const memory::Allocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage) {
    return memory::Buffer{allocator, size, usage, memory::Allocator::MemoryAccess::kGpuOnly};
}

/// @return The capacity of @p desc, checked first: the constructor's first step, before any buffer is created.
std::uint32_t validatedCapacity(const plugins::backend::graphics::ParticleSystemDesc& desc) {
    plugins::backend::graphics::validate(desc);
    return desc.capacity;
}

}  // namespace

GpuParticleSystem::GpuParticleSystem(const core::Device& device, const memory::Allocator& allocator,
                                     memory::Uploader& uploader, descriptor::DescriptorAllocator& descriptors,
                                     const descriptor::DescriptorSetLayout& setLayout,
                                     const plugins::backend::graphics::ParticleSystemDesc& desc)
    : _capacity{validatedCapacity(desc)},
      _sortCount{desc.sortByAge ? std::bit_ceil(_capacity) : 0},
      _params{makeBuffer(allocator, sizeof(GpuParticleParams),
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst)},
      _particles{makeBuffer(allocator, vk::DeviceSize{_capacity} * sizeof(GpuParticle),
                            vk::BufferUsageFlagBits::eStorageBuffer)},
      _dead{makeBuffer(allocator, vk::DeviceSize{_capacity} * sizeof(std::uint32_t),
                       vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst)},
      _alive{makeBuffer(allocator, vk::DeviceSize{_capacity} * 2 * sizeof(std::uint32_t),
                        vk::BufferUsageFlagBits::eStorageBuffer)},
      _counters{makeBuffer(allocator, sizeof(GpuParticleCounters),
                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                               vk::BufferUsageFlagBits::eTransferSrc)},
      _instances{makeBuffer(
          allocator,
          vk::DeviceSize{std::max(_capacity, _sortCount)} * sizeof(plugins::backend::graphics::ParticleInstance),
          vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer)},
      _indirect{makeBuffer(allocator, sizeof(vk::DrawIndirectCommand),
                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer |
                               vk::BufferUsageFlagBits::eTransferDst)},
      _set{descriptors.allocate(setLayout)} {
    const GpuParticleParams params = GpuParticleParams::fromDesc(desc);
    uploader.upload(std::as_bytes(std::span{&params, 1}), _params);
    std::vector<std::uint32_t> free;
    free.reserve(_capacity);
    for (std::uint32_t slot = 0; slot < _capacity; ++slot) {
        free.push_back(slot);
    }
    uploader.upload(std::as_bytes(std::span{free}), _dead);
    const GpuParticleCounters counters{.dead = static_cast<std::int32_t>(_capacity)};
    uploader.upload(std::as_bytes(std::span{&counters, 1}), _counters);
    const vk::DrawIndirectCommand command{kQuadVertexCount, 0, 0, 0};
    uploader.upload(std::as_bytes(std::span{&command, 1}), _indirect);

    const std::array<const memory::Buffer*, kBindingCount> buffers{&_params,   &_particles, &_dead,    &_alive,
                                                                   &_counters, &_instances, &_indirect};
    descriptor::DescriptorWriter writer;
    for (std::uint32_t binding = 0; binding < kBindingCount; ++binding) {
        const memory::Buffer& buffer = *buffers.at(binding);
        writer.writeBuffer(binding, *buffer.getBuffer(), 0, buffer.getSize(), vk::DescriptorType::eStorageBuffer);
    }
    writer.update(device, *_set);
}

}  // namespace rtype::render::vulkan::particle
