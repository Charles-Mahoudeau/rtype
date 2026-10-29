/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleConstants
*/

#include "ParticleConstants.hpp"

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "Blend.hpp"
#include "plugins/backend/graphics/Particle.hpp"

namespace rtype::render::vulkan::rendering {

ParticleConstants ParticleConstants::fromDraw(const plugins::backend::graphics::ParticleDraw& particles,
                                              glm::uvec2 textureSize) noexcept {
    ParticleConstants constants{.blend = toShaderBlend(particles.blend),
                                .alignment = static_cast<std::uint32_t>(particles.alignment)};
    const glm::vec2 sourceSize{particles.source.getWidth(), particles.source.getHeight()};
    if (sourceSize.x > 0.0F && sourceSize.y > 0.0F && textureSize.x > 0 && textureSize.y > 0) {
        const glm::vec2 texture{textureSize};
        const glm::vec2 uvMin = glm::vec2{particles.source.getX(), particles.source.getY()} / texture;
        const glm::vec2 uvMax = uvMin + (sourceSize / texture);
        constants.uvRect = {uvMin.x, uvMin.y, uvMax.x, uvMax.y};
    }
    return constants;
}

}  // namespace rtype::render::vulkan::rendering
