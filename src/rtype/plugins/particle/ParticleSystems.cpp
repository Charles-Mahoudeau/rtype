/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystems
*/

#include "plugins/particle/ParticleSystems.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <stdexcept>
#include <utility>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/ParticleRandom.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::particle {

ParticleSystems::Emitters& ParticleSystems::getEmitters() noexcept { return _emitters; }

const ParticleSystems::Emitters& ParticleSystems::getEmitters() const noexcept { return _emitters; }

std::size_t ParticleSystems::getCount() const noexcept { return _emitters.size(); }

EmitterState* ParticleSystems::find(ecs::Entity entity) {
    const auto found = _emitters.find(entity);
    return found != _emitters.end() ? &found->second : nullptr;
}

const EmitterState* ParticleSystems::find(ecs::Entity entity) const {
    const auto found = _emitters.find(entity);
    return found != _emitters.end() ? &found->second : nullptr;
}

EmitterState& ParticleSystems::start(ecs::Entity entity, EmitterState&& state) {
    const auto [stored, inserted] = _emitters.try_emplace(entity, std::move(state));
    if (!inserted) {
        throw std::logic_error{std::format(
            "ParticleSystems: the emitter of entity {} is already playing: stop it first", entity.getIndex())};
    }
    return stored->second;
}

std::optional<EmitterState> ParticleSystems::stop(ecs::Entity entity) {
    const auto found = _emitters.find(entity);
    if (found == _emitters.end()) {
        return std::nullopt;
    }
    std::optional<EmitterState> state{std::move(found->second)};
    _emitters.erase(found);
    return state;
}

std::uint32_t ParticleSystems::pickSeed() noexcept { return backend::graphics::pcgHash(++_seedCount); }

bool ParticleSystems::markFallbackReported(core::asset::AssetId effect) { return _fallbacks.insert(effect).second; }

}  // namespace rtype::plugins::particle
