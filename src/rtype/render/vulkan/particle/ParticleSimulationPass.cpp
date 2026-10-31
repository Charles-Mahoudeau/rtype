/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSimulationPass
*/

#include "ParticleSimulationPass.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "GpuParticleData.hpp"
#include "GpuParticleSystem.hpp"
#include "particle_emit.comp.h"
#include "particle_simulate.comp.h"
#include "particle_sort.comp.h"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/pipeline/ComputePipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::particle {

namespace {

/// @brief Threads of a workgroup: local_size_x of the particle compute shaders.
constexpr std::uint32_t kWorkgroupSize = 64;

/// @return Workgroups covering @p threads threads.
std::uint32_t groupsFor(std::uint32_t threads) noexcept { return (threads + kWorkgroupSize - 1) / kWorkgroupSize; }

/// @return The pipeline layout of the particle compute shaders.
pipeline::PipelineLayout createLayout(const core::Device& device, const descriptor::DescriptorSetLayout& setLayout) {
    const std::array setLayouts{*setLayout.getLayout()};
    const std::array pushConstants{
        vk::PushConstantRange{vk::ShaderStageFlagBits::eCompute, 0, sizeof(GpuParticlePush)}};
    return pipeline::PipelineLayout{device, setLayouts, pushConstants};
}

/// @brief Records a barrier making the writes of @p srcStage visible to the reads and writes of @p dstStage.
void barrier(const vk::raii::CommandBuffer& commandBuffer, vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
             vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess) {
    const vk::MemoryBarrier2 memory{srcStage, srcAccess, dstStage, dstAccess};
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{}.setMemoryBarriers(memory));
}

/// @return The push constants of @p job.
GpuParticlePush makePush(const ParticleSimulationPass::Job& job) noexcept {
    return GpuParticlePush{.emission = job.emission, .current = job.system->getCurrent()};
}

}  // namespace

ParticleSimulationPass::ParticleSimulationPass(const core::Device& device, pipeline::ShaderCache& shaders,
                                               const descriptor::DescriptorSetLayout& setLayout,
                                               const vk::raii::PipelineCache* cache)
    : _layout{createLayout(device, setLayout)},
      _emit{device, shaders.get("particle_emit.comp", rtype_shader_particle_emit_comp), _layout, nullptr, cache},
      _simulate{device, shaders.get("particle_simulate.comp", rtype_shader_particle_simulate_comp), _layout, nullptr,
                cache},
      _sort{device, shaders.get("particle_sort.comp", rtype_shader_particle_sort_comp), _layout, nullptr, cache} {}

void ParticleSimulationPass::record(const vk::raii::CommandBuffer& commandBuffer, std::span<const Job> jobs) {
    if (jobs.empty()) {
        return;
    }
    // 0. The previous frame's work on these buffers (its simulation, its copies, its draws reading the instances and
    // the commands) comes first.
    barrier(commandBuffer,
            vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eTransfer |
                vk::PipelineStageFlagBits2::eVertexAttributeInput | vk::PipelineStageFlagBits2::eDrawIndirect,
            vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite |
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);

    // 1. The other alive list starts empty.
    for (const Job& job : jobs) {
        const vk::DeviceSize next = 1 - job.system->getCurrent();
        commandBuffer.fillBuffer(*job.system->getCounters().getBuffer(),
                                 offsetof(GpuParticleCounters, alive) + (next * sizeof(std::uint32_t)),
                                 sizeof(std::uint32_t), 0);
    }
    barrier(commandBuffer, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);

    // 2. Emit.
    _emit.bind(commandBuffer);
    for (const Job& job : jobs) {
        if (job.emission.spawnCount == 0) {
            continue;
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *_layout.getLayout(), 0, job.system->getSet(),
                                         {});
        commandBuffer.pushConstants<GpuParticlePush>(*_layout.getLayout(), vk::ShaderStageFlagBits::eCompute, 0,
                                                     makePush(job));
        commandBuffer.dispatch(groupsFor(job.emission.spawnCount), 1, 1);
    }
    barrier(commandBuffer, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);

    // 3. Simulate.
    _simulate.bind(commandBuffer);
    for (const Job& job : jobs) {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *_layout.getLayout(), 0, job.system->getSet(),
                                         {});
        commandBuffer.pushConstants<GpuParticlePush>(*_layout.getLayout(), vk::ShaderStageFlagBits::eCompute, 0,
                                                     makePush(job));
        commandBuffer.dispatch(groupsFor(job.system->getCapacity()), 1, 1);
    }
    barrier(commandBuffer, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);
    recordSorts(commandBuffer, jobs);
    barrier(commandBuffer, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead);

    // 4. The alive count written this frame becomes the draw's instanceCount.
    for (const Job& job : jobs) {
        const vk::DeviceSize next = 1 - job.system->getCurrent();
        const vk::BufferCopy copy{offsetof(GpuParticleCounters, alive) + (next * sizeof(std::uint32_t)),
                                  offsetof(vk::DrawIndirectCommand, instanceCount), sizeof(std::uint32_t)};
        commandBuffer.copyBuffer(*job.system->getCounters().getBuffer(), *job.system->getIndirect().getBuffer(), copy);
        job.system->swapAliveLists();
    }

    // 5. Ready to be drawn.
    barrier(commandBuffer, vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eVertexAttributeInput | vk::PipelineStageFlagBits2::eDrawIndirect,
            vk::AccessFlagBits2::eVertexAttributeRead | vk::AccessFlagBits2::eIndirectCommandRead);
}

void ParticleSimulationPass::recordSorts(const vk::raii::CommandBuffer& commandBuffer, std::span<const Job> jobs) {
    std::uint32_t largest = 0;
    for (const Job& job : jobs) {
        largest = std::max(largest, job.system->getSortCount());
    }
    if (largest == 0) {
        return;
    }
    _sort.bind(commandBuffer);
    // k == 0 pads first; then every bitonic step, each system taking the steps of its own size.
    const auto step = [&](std::uint32_t j, std::uint32_t k) {
        for (const Job& job : jobs) {
            const std::uint32_t count = job.system->getSortCount();
            if (count == 0 || k > count) {
                continue;
            }
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *_layout.getLayout(), 0,
                                             job.system->getSet(), {});
            commandBuffer.pushConstants<GpuParticleSortPush>(
                *_layout.getLayout(), vk::ShaderStageFlagBits::eCompute, 0,
                GpuParticleSortPush{.j = j, .k = k, .count = count, .alive = 1 - job.system->getCurrent()});
            commandBuffer.dispatch(groupsFor(count), 1, 1);
        }
        barrier(commandBuffer, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite);
    };
    step(0, 0);
    for (std::uint32_t k = 2; k <= largest; k *= 2) {
        for (std::uint32_t j = k / 2; j > 0; j /= 2) {
            step(j, k);
        }
    }
}

}  // namespace rtype::render::vulkan::particle
