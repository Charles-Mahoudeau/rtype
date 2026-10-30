/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GpuParticleData
*/

#include "GpuParticleData.hpp"

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <variant>

#include "plugins/backend/graphics/ParticleCurve.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"

namespace rtype::render::vulkan::particle {

namespace {
/// @return @p range as (min, max).
glm::vec2 toVec2(const plugins::backend::graphics::Range& range) noexcept { return {range.min, range.max}; }
}  // namespace

GpuParticleParams GpuParticleParams::fromDesc(const plugins::backend::graphics::ParticleSystemDesc& desc) {
    GpuParticleParams params{.color = desc.color,
                             .gravity = desc.gravity,
                             .lifetime = toVec2(desc.lifetime),
                             .speed = toVec2(desc.speed),
                             .angle = toVec2(desc.angle),
                             .rotation = toVec2(desc.rotation),
                             .angularVelocity = toVec2(desc.angularVelocity),
                             .size = toVec2(desc.size),
                             .drag = desc.drag,
                             .space = static_cast<std::uint32_t>(desc.space),
                             .capacity = desc.capacity,
                             .colorOverLife = plugins::backend::graphics::bakeGradient(desc.colorOverLife),
                             .sizeOverLife = plugins::backend::graphics::bakeCurve(desc.sizeOverLife),
                             .speedOverLife = plugins::backend::graphics::bakeCurve(desc.speedOverLife)};
    if (const auto* circle = std::get_if<plugins::backend::graphics::CircleShape>(&desc.shape)) {
        params.shape = kShapeCircle;
        params.shapeSize = {circle->radius, 0.0F};
        params.edgeOnly = circle->edgeOnly ? 1 : 0;
    } else if (const auto* rect = std::get_if<plugins::backend::graphics::RectShape>(&desc.shape)) {
        params.shape = kShapeRect;
        params.shapeSize = rect->size;
    }
    return params;
}

}  // namespace rtype::render::vulkan::particle
