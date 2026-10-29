/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSpawn
*/

#include "plugins/particle/ParticleSpawn.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/particle/ParticleSystems.hpp"

namespace rtype::plugins::particle {
namespace {

/// @brief Fraction of a particle the rate's accumulator may miss and still emit it: float times are not exact
/// (1.0F - 0.8F is 0.1999999F), and without it a particle would be lost now and then. The accumulator then goes
/// slightly below 0, which the next frames make up.
constexpr float kRateTolerance = 1e-3F;

/// @return The particles the bursts of @p desc emit at the moments in [@p start, @p end).
std::uint64_t countBursts(const backend::graphics::ParticleSystemDesc& desc, float start, float end) {
    const float duration = desc.duration;
    const auto firstCycle = desc.loop ? static_cast<std::uint64_t>(std::floor(start / duration)) : 0U;
    const auto lastCycle = desc.loop ? static_cast<std::uint64_t>(std::floor(end / duration)) : 0U;
    std::uint64_t count = 0;
    for (std::uint64_t cycle = firstCycle; cycle <= lastCycle; ++cycle) {
        const float cycleStart = static_cast<float>(cycle) * duration;
        for (const backend::graphics::ParticleBurst& burst : desc.bursts) {
            for (std::uint32_t i = 0; i < burst.cycles; ++i) {
                const float local = burst.time + (static_cast<float>(i) * burst.interval);
                if (local >= duration) {
                    break;
                }
                const float moment = cycleStart + local;
                if (moment >= start && moment < end) {
                    count += burst.count;
                }
            }
        }
    }
    return count;
}

}  // namespace

std::uint32_t computeSpawnCount(const backend::graphics::ParticleSystemDesc& desc, float rateScale, float deltaTime,
                                EmitterState& state) {
    const float start = state.time;
    const float end = start + std::max(deltaTime, 0.0F);
    state.time = end;

    const float emitEnd = desc.loop ? end : std::min(end, desc.duration);
    const float emitStart = std::min(start, emitEnd);
    state.spawnAccumulator += desc.rate * std::max(rateScale, 0.0F) * (emitEnd - emitStart);
    const float fromRate = std::floor(state.spawnAccumulator + kRateTolerance);
    state.spawnAccumulator -= fromRate;

    const std::uint64_t count = static_cast<std::uint64_t>(fromRate) + countBursts(desc, start, end);
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(count, std::numeric_limits<std::uint32_t>::max()));
}

bool isEmissionOver(const backend::graphics::ParticleSystemDesc& desc, float time) noexcept {
    return !desc.loop && time >= desc.duration;
}

}  // namespace rtype::plugins::particle
