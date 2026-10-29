/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Particle
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <variant>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::backend::graphics {
/// @brief A particle system created by IRenderer::createParticleSystem(): a slot in the renderer's table and its
/// generation, as TextureId. Local to one renderer: never sent over the network.
struct ParticleSystemId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no system".

    [[nodiscard]] bool operator==(const ParticleSystemId& other) const = default;
};

/// @brief Where the particles of an effect are simulated.
enum class ParticleSimulation : std::uint8_t {
    kAuto,  ///< On the GPU if the renderer can, else on the CPU.
    kGpu,   ///< On the GPU; falls back to the CPU, with a warning, when the renderer cannot.
    kCpu,   ///< Always on the CPU: few particles that gameplay must read, or for debugging.
};

/// @brief What the particles of an emitter move in.
enum class ParticleSpace : std::uint8_t {
    kWorld,  ///< Left behind where they spawned: trails, smoke.
    kLocal,  ///< Follow the emitter: a shield's sparkles.
};

/// @brief How each particle's quad is turned.
enum class ParticleAlignment : std::uint8_t {
    kFixed,     ///< Drawn with its own rotation and size.
    kVelocity,  ///< Turned along its velocity and stretched by its speed: sparks, rain.
};

/// @brief One particle as it is drawn: what a CPU simulation hands the renderer, and what a GPU one keeps in its
/// buffers. The same layout on both sides, so the same vertex shader draws both.
struct ParticleInstance {
    Color color{1.0F, 1.0F, 1.0F, 1.0F};  ///< Multiplies the texture: tint and transparency.
    glm::vec2 position{0.0F, 0.0F};       ///< In world units.
    glm::vec2 velocity{0.0F, 0.0F};       ///< In world units per second; turns the quad with kVelocity.
    glm::vec2 size{1.0F, 1.0F};           ///< In world units.
    float rotation{0.0F};                 ///< In radians, clockwise on screen.
    float lifetime{1.0F};                 ///< Seconds the particle has left to live.
};

// Copied as is into GPU buffers and read by the particle shaders (std430): same size and field offsets as their struct.
static_assert(sizeof(ParticleInstance) == 48, "ParticleInstance must match the std430 layout of the shaders");
static_assert(offsetof(ParticleInstance, color) == 0, "ParticleInstance::color must match the shaders");
static_assert(offsetof(ParticleInstance, position) == 16, "ParticleInstance::position must match the shaders");
static_assert(offsetof(ParticleInstance, velocity) == 24, "ParticleInstance::velocity must match the shaders");
static_assert(offsetof(ParticleInstance, size) == 32, "ParticleInstance::size must match the shaders");
static_assert(offsetof(ParticleInstance, rotation) == 40, "ParticleInstance::rotation must match the shaders");
static_assert(offsetof(ParticleInstance, lifetime) == 44, "ParticleInstance::lifetime must match the shaders");

/// @brief One frame of an emitter, computed by the CPU: what a GPU system needs to emit and move its particles, given
/// to IRenderer::simulate().
struct ParticleEmission {
    glm::vec2 position{0.0F, 0.0F};  ///< The emitter's GlobalTransform, in world units: where particles spawn.
    glm::vec2 scale{1.0F, 1.0F};     ///< The emitter's GlobalTransform: scales the emission shape.
    float rotation{0.0F};            ///< The emitter's GlobalTransform, in radians: turns the shape.
    float deltaTime{0.0F};           ///< Seconds since the previous frame.
    std::uint32_t seed{0};           ///< The emitter's seed; the random numbers of its particles depend on it.
    /// Particles the emitter emitted before this frame: particle i of the frame draws the random numbers of particle
    /// firstParticle + i, so no two particles of an emitter share them, on the CPU and the GPU alike.
    std::uint32_t firstParticle{0};
    std::uint32_t spawnCount{0};  ///< Particles to emit this frame (rate and bursts, counted by the plugin).
};

/// @brief The particles of one emitter to draw this frame, as a Sprite is one sprite: where they are, and how they
/// look. Drawn by IRenderer::draw(), in the order of the sprites.
struct ParticleDraw {
    /// A GPU system, whose particles never leave the renderer, or the instances a CPU simulation hands over (not owned:
    /// they must stay valid until the renderer has drawn them).
    std::variant<ParticleSystemId, std::span<const ParticleInstance>> particles;
    TextureId texture;                      ///< Not owned, as for Sprite.
    math::Rect source;                      ///< Part of the texture each particle shows, in pixels.
    BlendMode blend{BlendMode::kAdditive};  ///< How the particles are combined with what is under them.
    ParticleAlignment alignment{ParticleAlignment::kFixed};  ///< How each particle's quad is turned.
};

}  // namespace rtype::plugins::backend::graphics
