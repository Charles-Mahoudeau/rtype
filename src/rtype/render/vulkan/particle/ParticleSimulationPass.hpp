/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSimulationPass
*/

#pragma once

#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "GpuParticleSystem.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/pipeline/ComputePipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::particle {

/// @brief Simulates the GPU particle systems of a frame: shaders/particle_emit.comp then particle_simulate.comp for
/// each, then the alive counts copied into their indirect commands, ready to be drawn.
///
/// @details Recorded before rendering begins (a dispatch cannot be recorded inside a rendering scope). Every system's
/// step is batched with the others', so a frame needs the same six barriers whatever the number of systems. The
/// previous frame's work on the buffers (its simulation, copies and draws) is waited for first, then:
/// 1. Reset the count of each system's other alive list (the one written this frame).
/// 2. Emit: one thread per particle to spawn.
/// 3. Simulate: one thread per slot of the capacity; those past the alive count return at once. Then, for the systems
///    sorted by age, pad the instances past the live ones and sort them by life (shaders/particle_sort.comp, one
///    bitonic step per dispatch, the steps of every sorted system batched).
/// 4. Copy each alive count into the instanceCount of the system's indirect command.
/// 5. Make the instances and the commands visible to the vertex input and the indirect draw.
/// Each system's alive lists are then swapped.
/// @warning Destroy it before the Device. The set layout given at construction must outlive it.
class RTYPE_RENDER_VULKAN_API ParticleSimulationPass {
  public:
    /// @brief One system to simulate this frame, with its emission.
    struct Job {
        GpuParticleSystem* system = nullptr;                    ///< Not owned.
        plugins::backend::graphics::ParticleEmission emission;  ///< What it emits and how far it moves.
    };

    /// @param shaders Gives the built-in particle compute shaders.
    /// @param setLayout The layout of a system's set (seven storage buffers).
    /// @param cache Speeds up the pipelines' creation, or null.
    /// @throws vk::SystemError If a pipeline cannot be created.
    ParticleSimulationPass(const core::Device& device, pipeline::ShaderCache& shaders,
                           const descriptor::DescriptorSetLayout& setLayout, const vk::raii::PipelineCache* cache);
    ~ParticleSimulationPass() = default;
    ParticleSimulationPass(const ParticleSimulationPass&) = delete;
    ParticleSimulationPass& operator=(const ParticleSimulationPass&) = delete;
    ParticleSimulationPass(ParticleSimulationPass&&) = delete;
    ParticleSimulationPass& operator=(ParticleSimulationPass&&) = delete;

    /// @brief Records the simulation of @p jobs, outside any rendering scope, and swaps their alive lists. Does
    /// nothing without a job.
    void record(const vk::raii::CommandBuffer& commandBuffer, std::span<const Job> jobs);

  private:
    /// @brief Records the padding and the bitonic sort of the sorted systems of @p jobs, a barrier after each step.
    void recordSorts(const vk::raii::CommandBuffer& commandBuffer, std::span<const Job> jobs);

    pipeline::PipelineLayout _layout;     ///< Set 0: a system's buffers; push constants: GpuParticlePush.
    pipeline::ComputePipeline _emit;      ///< shaders/particle_emit.comp.
    pipeline::ComputePipeline _simulate;  ///< shaders/particle_simulate.comp.
    pipeline::ComputePipeline _sort;      ///< shaders/particle_sort.comp.
};

}  // namespace rtype::render::vulkan::particle
