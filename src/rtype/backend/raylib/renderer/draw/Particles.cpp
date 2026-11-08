/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Particles
*/

#include "Particles.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <span>

#include "backend/raylib/renderer/Convert.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"

namespace rtype::backend::raylib::renderer::draw {
namespace {

/// @brief The grid of a flipbook in a source rectangle.
struct Flipbook {
    ::Rectangle whole;      ///< The source rectangle the grid covers.
    std::uint32_t columns;  ///< Frames per row, at least 1.
    std::uint32_t frames;   ///< Frames played, at least 1.
    float cycles;           ///< Times the animation plays over a life.
    glm::vec2 cell;         ///< The size of one frame, in pixels.

    /// @return The source rectangle of the frame shown at @p life (0 at birth, 1 at death).
    [[nodiscard]] ::Rectangle frameAt(float life) const {
        const float progress = std::fmod(std::clamp(life, 0.0F, 1.0F) * cycles, 1.0F);
        const std::uint32_t frame =
            std::min(static_cast<std::uint32_t>(progress * static_cast<float>(frames)), frames - 1);
        const std::uint32_t column = frame % columns;
        const std::uint32_t row = frame / columns;
        return {.x = whole.x + (static_cast<float>(column) * cell.x),
                .y = whole.y + (static_cast<float>(row) * cell.y),
                .width = cell.x,
                .height = cell.y};
    }
};

Flipbook flipbookOf(const ::Texture2D& texture, const contract::renderer::ParticleDraw& particles) {
    const contract::renderer::ParticleFlipbook& book = particles.flipbook;
    const ::Rectangle whole = sourceOrWhole(particles.source, texture);
    const std::uint32_t columns = std::max(book.columns, 1U);
    const std::uint32_t rows = std::max(book.rows, 1U);
    return {.whole = whole,
            .columns = columns,
            .frames = std::max(book.frameCount != 0 ? book.frameCount : columns * rows, 1U),
            .cycles = book.cycles,
            .cell = {whole.width / static_cast<float>(columns), whole.height / static_cast<float>(rows)}};
}

}  // namespace

void drawParticles(const ::Texture2D& texture, std::span<const contract::renderer::ParticleInstance> instances,
                   const contract::renderer::ParticleDraw& particles) {
    const Flipbook flipbook = flipbookOf(texture, particles);
    const bool alongVelocity = particles.alignment == contract::renderer::ParticleAlignment::kVelocity;
    for (const contract::renderer::ParticleInstance& particle : instances) {
        glm::vec2 size = particle.size;
        float angle = particle.rotation;
        if (alongVelocity) {
            const float speed = glm::length(particle.velocity);
            angle = speed > 0.0F ? std::atan2(particle.velocity.y, particle.velocity.x) : 0.0F;
            size.x += speed * particles.stretch;
        }
        const ::Rectangle destination{
            .x = particle.position.x, .y = particle.position.y, .width = size.x, .height = size.y};
        DrawTexturePro(texture, flipbook.frameAt(particle.life), destination, toVector(size / 2.0F),
                       angle * kDegreesPerRadian, toColor(particle.color));
    }
}

}  // namespace rtype::backend::raylib::renderer::draw
