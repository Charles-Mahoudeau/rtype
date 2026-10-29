/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CpuSimulation
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <span>
#include <vector>

#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleCurve.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"

namespace rtype::plugins::particle {

/// @brief The particles of one emitter, simulated on the CPU: the reference the GPU simulation must match
/// (docs/particles/design.md), and its fallback when the renderer has none.
///
/// @details Plain C++ without the ECS. Each step() emits the particles of the frame, then moves, ages and kills every
/// particle; the live ones stay packed in getInstances(), ready to be drawn. One step, in this order:
/// - **Emit** ParticleEmission::spawnCount particles, dropped beyond the capacity. Particle i of the frame draws the
///   random numbers of particle `firstParticle + i` (particleRandom()): its lifetime, speed, direction, rotation,
///   angular velocity, size, and its place in the shape. The shape is scaled then turned by the emitter, and so is the
///   direction (`angle` plus the emitter's rotation).
/// - **Age** every particle by the frame's deltaTime; one whose age reaches its lifetime dies, replaced by the last.
/// - **Move** the others: `velocity += gravity * dt`, `velocity *= exp(-drag * dt)`, `position += velocity *
///   speedOverLife(t) * dt`, `rotation += angularVelocity * dt`, where t is age / lifetime.
/// - **Look**: colour `color * colorOverLife(t)`, size `size * sizeOverLife(t)`.
///
/// With ParticleSpace::kLocal, particles keep their position relative to the emitter, and follow it when it moves (not
/// when it turns); with kWorld, they stay where they were emitted.
class CpuSimulation {
  public:
    /// @brief Bakes the curves of @p desc, and reserves its capacity.
    /// @throws backend::exceptions::ParticleException If @p desc is invalid (backend::graphics::validate()).
    explicit CpuSimulation(backend::graphics::ParticleSystemDesc desc);
    ~CpuSimulation() = default;
    CpuSimulation(const CpuSimulation& other) = default;
    CpuSimulation& operator=(const CpuSimulation& other) = default;
    CpuSimulation(CpuSimulation&& other) noexcept = default;
    CpuSimulation& operator=(CpuSimulation&& other) noexcept = default;

    /// @return The live particles, as they are drawn: valid until the next step() or clear().
    [[nodiscard]] std::span<const backend::graphics::ParticleInstance> getInstances() const noexcept;

    /// @return The number of live particles.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The most particles alive at once.
    [[nodiscard]] std::uint32_t getCapacity() const noexcept;

    /// @brief Advances by one frame: emits, ages, kills and moves the particles, as the class describes.
    void step(const backend::graphics::ParticleEmission& emission);

    /// @brief Kills every particle at once.
    void clear() noexcept;

  private:
    /// @brief What a particle keeps between frames, besides what is drawn.
    struct Particle {
        glm::vec2 position{0.0F, 0.0F};  ///< In the world (kWorld), or relative to the emitter (kLocal).
        glm::vec2 velocity{0.0F, 0.0F};  ///< World units per second, before speedOverLife.
        float rotation{0.0F};            ///< Radians, clockwise on screen.
        float angularVelocity{0.0F};     ///< Radians per second.
        float size{1.0F};                ///< Width and height at birth, before sizeOverLife.
        float age{0.0F};                 ///< Seconds since it was emitted.
        float lifetime{1.0F};            ///< Seconds it lives.
    };

    /// @brief Emits @p emission.spawnCount particles, as many as the capacity leaves room for.
    void emit(const backend::graphics::ParticleEmission& emission);

    /// @return Particle @p index of the emitter, born at the emitter of @p emission.
    [[nodiscard]] Particle spawn(const backend::graphics::ParticleEmission& emission, std::uint32_t index) const;

    /// @return Where particle @p index spawns in the shape, relative to the emitter, before its scale and rotation.
    [[nodiscard]] glm::vec2 pickShapeOffset(std::uint32_t seed, std::uint32_t index) const;

    /// @brief Ages, kills and moves every particle by @p deltaTime.
    void update(float deltaTime);

    /// @brief Rewrites the drawn instances from the particles, placed at @p emitterPosition for kLocal.
    void writeInstances(glm::vec2 emitterPosition);

    backend::graphics::ParticleSystemDesc _desc;                  ///< What is simulated.
    backend::graphics::BakedGradient _colorOverLife{};            ///< _desc.colorOverLife, baked.
    backend::graphics::BakedCurve _sizeOverLife{};                ///< _desc.sizeOverLife, baked.
    backend::graphics::BakedCurve _speedOverLife{};               ///< _desc.speedOverLife, baked.
    std::vector<Particle> _particles;                             ///< The live particles, packed.
    std::vector<backend::graphics::ParticleInstance> _instances;  ///< _particles as drawn, in the same order.
};

}  // namespace rtype::plugins::particle
