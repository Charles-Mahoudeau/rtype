/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSpawn
*/

#pragma once

#include <cstdint>

#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/particle/ParticleSystems.hpp"

namespace rtype::plugins::particle {

/// @brief Counts the particles an emitter emits during the next @p deltaTime seconds, and advances its clock: the CPU
/// decides how many particles spawn on both paths, so the CPU and GPU simulations spawn the same at the same frames.
///
/// @details The frame covers [state.time, state.time + deltaTime), state.time counting from the start without
/// wrapping. A cycle lasts desc.duration; with desc.loop, cycle k starts at k * duration, without it there is one
/// cycle.
/// - **Rate:** desc.rate * @p rateScale particles per second while a cycle plays, the fraction left carried to the next
///   frame in state.spawnAccumulator.
/// - **Bursts:** a burst emits its count at `time + i * interval` of each cycle, for i below its cycles, if that
///   moment is inside the cycle (below duration): later ones never fire.
/// The count is not capped by the capacity: the simulations drop what does not fit.
/// @param rateScale Multiplies the rate (ParticleEmitter::rateScale); a negative one counts as 0. Bursts ignore it.
/// @return The particles to emit this frame (ParticleEmission::spawnCount).
[[nodiscard]] std::uint32_t computeSpawnCount(const backend::graphics::ParticleSystemDesc& desc, float rateScale,
                                              float deltaTime, EmitterState& state);

/// @return Whether an emitter at @p time has emitted everything it will: never with desc.loop, else once its one cycle
/// is over. Its particles may still be alive.
[[nodiscard]] bool isEmissionOver(const backend::graphics::ParticleSystemDesc& desc, float time) noexcept;

}  // namespace rtype::plugins::particle
