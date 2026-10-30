/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GpuParticleData
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleCurve.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "render/vulkan/Export.hpp"

/// @brief The data of a GPU particle system as its compute shaders read it: the CPU side of
/// shaders/include/ParticleSystem.glsl, which must match it field by field (std430 layout). Change both files together;
/// the static_asserts below keep the offsets in step.
namespace rtype::render::vulkan::particle {

/// @brief Bindings of a system's buffers, in set 0 of the particle compute shaders.
/// @{
inline constexpr std::uint32_t kParamsBinding = 0;     ///< GpuParticleParams, read only.
inline constexpr std::uint32_t kParticlesBinding = 1;  ///< GpuParticle per slot, capacity slots.
inline constexpr std::uint32_t kDeadBinding = 2;       ///< Indices of the free slots.
inline constexpr std::uint32_t kAliveBinding = 3;      ///< Two lists of live slots, capacity each.
inline constexpr std::uint32_t kCountersBinding = 4;   ///< GpuParticleCounters.
inline constexpr std::uint32_t kInstancesBinding = 5;  ///< The live particles as drawn, packed.
inline constexpr std::uint32_t kIndirectBinding = 6;   ///< The VkDrawIndirectCommand of the draw.
inline constexpr std::uint32_t kBindingCount = 7;      ///< Number of bindings.
/// @}

/// @brief How a system spawns its particles: the alternative of backend::graphics::EmitterShape.
/// @{
inline constexpr std::uint32_t kShapePoint = 0;
inline constexpr std::uint32_t kShapeCircle = 1;
inline constexpr std::uint32_t kShapeRect = 2;
/// @}

static_assert(static_cast<std::uint32_t>(plugins::backend::graphics::ParticleSpace::kWorld) == 0 &&
                  static_cast<std::uint32_t>(plugins::backend::graphics::ParticleSpace::kLocal) == 1,
              "graphics::ParticleSpace must keep the values of ParticleSystem.glsl");

/// @brief A ParticleSystemDesc as the compute shaders read it, with its curves baked: one storage buffer per system,
/// written once at creation. 1648 bytes.
struct GpuParticleParams {
    plugins::backend::graphics::Color color{1.0F};              ///< Offset 0.
    glm::vec2 gravity{0.0F};                                    ///< Offset 16.
    glm::vec2 lifetime{1.0F};                                   ///< (min, max). Offset 24.
    glm::vec2 speed{0.0F};                                      ///< (min, max). Offset 32.
    glm::vec2 angle{0.0F};                                      ///< (min, max). Offset 40.
    glm::vec2 rotation{0.0F};                                   ///< (min, max). Offset 48.
    glm::vec2 angularVelocity{0.0F};                            ///< (min, max). Offset 56.
    glm::vec2 size{1.0F};                                       ///< (min, max). Offset 64.
    glm::vec2 shapeSize{0.0F};                                  ///< Rect: its size; circle: (radius, 0). Offset 72.
    float drag = 0.0F;                                          ///< Offset 80.
    std::uint32_t shape = kShapePoint;                          ///< kShapePoint, kShapeCircle or kShapeRect. Offset 84.
    std::uint32_t edgeOnly = 0;                                 ///< Circle: 1 to spawn on the circle only. Offset 88.
    std::uint32_t space = 0;                                    ///< graphics::ParticleSpace. Offset 92.
    std::uint32_t capacity = 0;                                 ///< Most particles alive at once. Offset 96.
    std::array<std::uint32_t, 3> padding{};                     ///< Aligns colorOverLife to 16 bytes, as std430 does.
    plugins::backend::graphics::BakedGradient colorOverLife{};  ///< Offset 112.
    plugins::backend::graphics::BakedCurve sizeOverLife{};      ///< Offset 1136.
    plugins::backend::graphics::BakedCurve speedOverLife{};     ///< Offset 1392.

    /// @return @p desc as the compute shaders read it, its curves baked.
    /// @throws plugins::backend::exceptions::ParticleException If a curve of @p desc is invalid.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static GpuParticleParams fromDesc(
        const plugins::backend::graphics::ParticleSystemDesc& desc);
};
static_assert(offsetof(GpuParticleParams, gravity) == 16 && offsetof(GpuParticleParams, lifetime) == 24 &&
                  offsetof(GpuParticleParams, shapeSize) == 72 && offsetof(GpuParticleParams, drag) == 80 &&
                  offsetof(GpuParticleParams, capacity) == 96 && offsetof(GpuParticleParams, colorOverLife) == 112 &&
                  offsetof(GpuParticleParams, sizeOverLife) == 1136 &&
                  offsetof(GpuParticleParams, speedOverLife) == 1392 && sizeof(GpuParticleParams) == 1648,
              "GpuParticleParams must match the std430 layout of ParticleSystem.glsl");

/// @brief What a slot keeps between frames, as CpuSimulation's particle does: 40 bytes.
struct GpuParticle {
    glm::vec2 position{0.0F};      ///< In the world (kWorld), or relative to the emitter (kLocal). Offset 0.
    glm::vec2 velocity{0.0F};      ///< World units per second, before speedOverLife. Offset 8.
    float rotation = 0.0F;         ///< Radians. Offset 16.
    float angularVelocity = 0.0F;  ///< Radians per second. Offset 20.
    float size = 1.0F;             ///< At birth, before sizeOverLife. Offset 24.
    float age = 0.0F;              ///< Seconds since it was emitted. Offset 28.
    float lifetime = 1.0F;         ///< Seconds it lives. Offset 32.
    float padding = 0.0F;          ///< Rounds the size to 8 bytes, as std430 does.
};
static_assert(offsetof(GpuParticle, rotation) == 16 && offsetof(GpuParticle, lifetime) == 32 &&
                  sizeof(GpuParticle) == 40,
              "GpuParticle must match the std430 layout of ParticleSystem.glsl");

/// @brief The counters of a system's lists: 16 bytes.
struct GpuParticleCounters {
    std::int32_t dead = 0;                 ///< Free slots in the dead list. Offset 0.
    std::array<std::uint32_t, 2> alive{};  ///< Live slots in each alive list. Offset 4.
    std::uint32_t padding = 0;             ///< Rounds the size to 16 bytes.
};
static_assert(offsetof(GpuParticleCounters, alive) == 4 && sizeof(GpuParticleCounters) == 16,
              "GpuParticleCounters must match the std430 layout of ParticleSystem.glsl");

/// @brief The push constants of the particle compute shaders: the frame's emission, and which alive list is read
/// (the other is written). 40 bytes.
struct GpuParticlePush {
    plugins::backend::graphics::ParticleEmission emission;  ///< Offset 0.
    std::uint32_t current = 0;                              ///< Alive list read this frame: 0 or 1. Offset 36.
};
static_assert(offsetof(plugins::backend::graphics::ParticleEmission, scale) == 8 &&
                  offsetof(plugins::backend::graphics::ParticleEmission, rotation) == 16 &&
                  offsetof(plugins::backend::graphics::ParticleEmission, deltaTime) == 20 &&
                  offsetof(plugins::backend::graphics::ParticleEmission, seed) == 24 &&
                  offsetof(plugins::backend::graphics::ParticleEmission, firstParticle) == 28 &&
                  offsetof(plugins::backend::graphics::ParticleEmission, spawnCount) == 32 &&
                  offsetof(GpuParticlePush, current) == 36 && sizeof(GpuParticlePush) == 40,
              "GpuParticlePush must match the push constants of ParticleSystem.glsl");

/// @brief The push constants of shaders/particle_sort.comp: one step of the bitonic sort of a system's instances by
/// life, or (k == 0) the padding of the slots past the live particles. 16 bytes.
struct GpuParticleSortPush {
    std::uint32_t j = 0;      ///< Distance between the two instances compared. Offset 0.
    std::uint32_t k = 0;      ///< Size of the sequences being merged; 0: pad. Offset 4.
    std::uint32_t count = 0;  ///< Instances sorted: the capacity rounded up to a power of two. Offset 8.
    std::uint32_t alive = 0;  ///< Alive list this frame's simulation wrote, whose count is the live one. Offset 12.
};
static_assert(offsetof(GpuParticleSortPush, alive) == 12 && sizeof(GpuParticleSortPush) <= sizeof(GpuParticlePush),
              "GpuParticleSortPush must match particle_sort.comp and fit the particle push constant range");

}  // namespace rtype::render::vulkan::particle
