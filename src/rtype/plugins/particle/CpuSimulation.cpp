/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CpuSimulation
*/

#include "plugins/particle/CpuSimulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <numbers>
#include <span>
#include <utility>
#include <variant>

#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleCurve.hpp"
#include "rtype/contract/renderer/particle/ParticleRandom.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"

namespace rtype::plugins::particle {
namespace {

using contract::renderer::ParticleRandomStream;

/// @return A value of @p range for particle @p index, from its random number @p stream.
float pick(const contract::renderer::Range& range, std::uint32_t seed, std::uint32_t index,
           ParticleRandomStream stream) {
    return range.min + ((range.max - range.min) * contract::renderer::particleRandomFloat(seed, index, stream));
}

/// @return @p vector turned by @p angle radians, clockwise on screen (Y down).
glm::vec2 rotate(glm::vec2 vector, float angle) {
    const float cos = std::cos(angle);
    const float sin = std::sin(angle);
    return glm::vec2{(vector.x * cos) - (vector.y * sin), (vector.x * sin) + (vector.y * cos)};
}

}  // namespace

CpuSimulation::CpuSimulation(contract::renderer::ParticleSystemDesc desc) : _desc{std::move(desc)} {
    contract::renderer::validate(_desc);
    _colorOverLife = contract::renderer::bakeGradient(_desc.colorOverLife);
    _sizeOverLife = contract::renderer::bakeCurve(_desc.sizeOverLife);
    _speedOverLife = contract::renderer::bakeCurve(_desc.speedOverLife);
    _particles.reserve(_desc.capacity);
    _instances.reserve(_desc.capacity);
}

std::span<const contract::renderer::ParticleInstance> CpuSimulation::getInstances() const noexcept {
    return _instances;
}

std::size_t CpuSimulation::getCount() const noexcept { return _particles.size(); }

std::uint32_t CpuSimulation::getCapacity() const noexcept { return _desc.capacity; }

void CpuSimulation::step(const contract::renderer::ParticleEmission& emission) {
    emit(emission);
    update(emission.deltaTime);
    writeInstances(emission.position);
}

void CpuSimulation::clear() noexcept {
    _particles.clear();
    _instances.clear();
}

void CpuSimulation::emit(const contract::renderer::ParticleEmission& emission) {
    const std::size_t room = _desc.capacity - _particles.size();
    const auto count = static_cast<std::uint32_t>(std::min<std::size_t>(emission.spawnCount, room));
    for (std::uint32_t i = 0; i < count; ++i) {
        _particles.push_back(spawn(emission, emission.firstParticle + i));
    }
}

CpuSimulation::Particle CpuSimulation::spawn(const contract::renderer::ParticleEmission& emission,
                                             std::uint32_t index) const {
    const std::uint32_t seed = emission.seed;
    const glm::vec2 offset = rotate(pickShapeOffset(seed, index) * emission.scale, emission.rotation);
    const float angle = pick(_desc.angle, seed, index, ParticleRandomStream::kAngle) + emission.rotation;
    const float speed = pick(_desc.speed, seed, index, ParticleRandomStream::kSpeed);

    Particle particle;
    particle.position = _desc.space == contract::renderer::ParticleSpace::kLocal ? offset : emission.position + offset;
    particle.velocity = glm::vec2{std::cos(angle), std::sin(angle)} * speed;
    particle.rotation = pick(_desc.rotation, seed, index, ParticleRandomStream::kRotation);
    particle.angularVelocity = pick(_desc.angularVelocity, seed, index, ParticleRandomStream::kAngularVelocity);
    particle.size = pick(_desc.size, seed, index, ParticleRandomStream::kSize);
    particle.lifetime = pick(_desc.lifetime, seed, index, ParticleRandomStream::kLifetime);
    return particle;
}

glm::vec2 CpuSimulation::pickShapeOffset(std::uint32_t seed, std::uint32_t index) const {
    const float randomX = contract::renderer::particleRandomFloat(seed, index, ParticleRandomStream::kShapeX);
    const float randomY = contract::renderer::particleRandomFloat(seed, index, ParticleRandomStream::kShapeY);
    if (const auto* circle = std::get_if<contract::renderer::CircleShape>(&_desc.shape)) {
        const float angle = 2.0F * std::numbers::pi_v<float> * randomX;
        const float distance = circle->edgeOnly ? circle->radius : circle->radius * std::sqrt(randomY);
        return glm::vec2{std::cos(angle), std::sin(angle)} * distance;
    }
    if (const auto* rect = std::get_if<contract::renderer::RectShape>(&_desc.shape)) {
        return (glm::vec2{randomX, randomY} - 0.5F) * rect->size;
    }
    return glm::vec2{0.0F, 0.0F};
}

void CpuSimulation::update(float deltaTime) {
    const float dragFactor = std::exp(-_desc.drag * deltaTime);
    std::size_t i = 0;
    while (i < _particles.size()) {
        Particle& particle = _particles.at(i);
        particle.age += deltaTime;
        if (particle.age >= particle.lifetime) {
            particle = _particles.back();
            _particles.pop_back();
            continue;
        }
        const float t = particle.age / particle.lifetime;
        particle.velocity = (particle.velocity + (_desc.gravity * deltaTime)) * dragFactor;
        particle.position += particle.velocity * contract::renderer::sampleBaked(_speedOverLife, t) * deltaTime;
        particle.rotation += particle.angularVelocity * deltaTime;
        ++i;
    }
}

void CpuSimulation::writeInstances(glm::vec2 emitterPosition) {
    const bool local = _desc.space == contract::renderer::ParticleSpace::kLocal;
    _instances.resize(_particles.size());
    for (std::size_t i = 0; i < _particles.size(); ++i) {
        const Particle& particle = _particles.at(i);
        const float t = particle.age / particle.lifetime;
        contract::renderer::ParticleInstance& instance = _instances.at(i);
        instance.color = _desc.color * contract::renderer::sampleBaked(_colorOverLife, t);
        instance.position = local ? emitterPosition + particle.position : particle.position;
        instance.velocity = particle.velocity * contract::renderer::sampleBaked(_speedOverLife, t);
        instance.size = glm::vec2{particle.size * contract::renderer::sampleBaked(_sizeOverLife, t)};
        instance.rotation = particle.rotation;
        instance.life = t;
    }
    if (_desc.sortByAge) {
        std::ranges::sort(_instances,
                          [](const contract::renderer::ParticleInstance& left,
                             const contract::renderer::ParticleInstance& right) { return left.life > right.life; });
    }
}

}  // namespace rtype::plugins::particle
