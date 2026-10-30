/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GpuParticleSystem
*/

#pragma once

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/memory/Uploader.hpp"

namespace rtype::render::vulkan::particle {

/// @brief One GPU particle system, behind a ParticleSystemId: the buffers its compute shaders simulate it in
/// (GpuParticleData.hpp, ParticleSystem.glsl), sized by its capacity, and the descriptor set that binds them.
///
/// @details Every buffer lives in device local memory: only the GPU reads and writes them after creation. The
/// parameters, the dead list (every slot free), the counters and the indirect command (6 vertices, no instance) are
/// filled through the Uploader, so the caller must flush it before the first simulation. The instances are drawn as a
/// vertex buffer and the indirect command by vkCmdDrawIndirect.
/// @warning Destroy it before the Allocator, the DescriptorAllocator and the Device, once the GPU is done with it.
class RTYPE_RENDER_VULKAN_API GpuParticleSystem {
  public:
    /// @brief Creates the buffers of @p desc's capacity, queues their first content into @p uploader and allocates
    /// the descriptor set of @p setLayout (ParticleSystem.glsl's set 0) from @p descriptors.
    /// @throws plugins::backend::exceptions::ParticleException If @p desc is invalid.
    /// @throws vk::SystemError If a buffer or the set cannot be created.
    GpuParticleSystem(const core::Device& device, const memory::Allocator& allocator, memory::Uploader& uploader,
                      descriptor::DescriptorAllocator& descriptors, const descriptor::DescriptorSetLayout& setLayout,
                      const plugins::backend::graphics::ParticleSystemDesc& desc);
    ~GpuParticleSystem() = default;
    GpuParticleSystem(const GpuParticleSystem&) = delete;
    GpuParticleSystem& operator=(const GpuParticleSystem&) = delete;
    GpuParticleSystem(GpuParticleSystem&&) noexcept = default;
    GpuParticleSystem& operator=(GpuParticleSystem&&) noexcept = default;

    /// @return Most particles alive at once.
    [[nodiscard]] std::uint32_t getCapacity() const noexcept { return _capacity; }
    /// @return The alive list read by the next simulation: 0 or 1.
    [[nodiscard]] std::uint32_t getCurrent() const noexcept { return _current; }
    /// @return The descriptor set binding every buffer, set 0 of the particle compute shaders.
    [[nodiscard]] vk::DescriptorSet getSet() const noexcept { return *_set; }
    /// @return The counters, whose alive count is copied into the indirect command.
    [[nodiscard]] const memory::Buffer& getCounters() const noexcept { return _counters; }
    /// @return The live particles as drawn, packed: the draw's vertex buffer.
    [[nodiscard]] const memory::Buffer& getInstances() const noexcept { return _instances; }
    /// @return The VkDrawIndirectCommand of the draw.
    [[nodiscard]] const memory::Buffer& getIndirect() const noexcept { return _indirect; }

    /// @brief Makes the alive list written by the last simulation the one read by the next.
    void swapAliveLists() noexcept { _current = 1 - _current; }

  private:
    std::uint32_t _capacity;       ///< Most particles alive at once.
    std::uint32_t _current = 0;    ///< Alive list read by the next simulation.
    memory::Buffer _params;        ///< GpuParticleParams.
    memory::Buffer _particles;     ///< GpuParticle per slot.
    memory::Buffer _dead;          ///< Indices of the free slots.
    memory::Buffer _alive;         ///< Two lists of live slots, capacity each.
    memory::Buffer _counters;      ///< GpuParticleCounters.
    memory::Buffer _instances;     ///< The live particles as drawn, packed.
    memory::Buffer _indirect;      ///< VkDrawIndirectCommand.
    vk::raii::DescriptorSet _set;  ///< Binds every buffer above.
};

}  // namespace rtype::render::vulkan::particle
