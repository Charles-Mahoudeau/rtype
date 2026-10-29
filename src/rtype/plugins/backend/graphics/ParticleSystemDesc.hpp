/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystemDesc
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <numbers>
#include <variant>
#include <vector>

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleCurve.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief A value picked per particle, uniformly between min and max (both included). min == max: always that value.
struct Range {
    float min{0.0F};  ///< Smallest value.
    float max{0.0F};  ///< Largest value.
};

/// @brief Particles emitted at once, at a moment of each cycle of the emitter: explosions, shots.
struct ParticleBurst {
    float time{0.0F};         ///< Seconds after the start of the cycle.
    std::uint32_t count{0};   ///< Particles emitted each time.
    std::uint32_t cycles{1};  ///< How many times it happens: 1 is once.
    float interval{0.0F};     ///< Seconds between two times, when cycles is above 1.
};

/// @brief Particles spawn on the emitter's position.
struct PointShape {};

/// @brief Particles spawn in a circle centred on the emitter.
struct CircleShape {
    float radius{0.0F};    ///< In world units.
    bool edgeOnly{false};  ///< true: on the circle only, as a ring; false: anywhere inside it.
};

/// @brief Particles spawn in a rectangle centred on the emitter.
struct RectShape {
    glm::vec2 size{0.0F, 0.0F};  ///< Width and height, in world units.
};

/// @brief Where the particles of an effect spawn, around the emitter, turned and scaled with it. Which way they go is
/// ParticleSystemDesc::angle: a cone or an arc is a shape and an angle range.
using EmitterShape = std::variant<PointShape, CircleShape, RectShape>;

/// @brief How the particles of an effect spawn, move and age: what a simulation, CPU or GPU, runs. Given to
/// IRenderer::createParticleSystem(); how they look (texture, blend) is given with each draw.
struct ParticleSystemDesc {
    ParticleSimulation simulation{ParticleSimulation::kAuto};  ///< Where it runs.
    std::uint32_t capacity{1024};                              ///< Most particles alive at once.
    ParticleSpace space{ParticleSpace::kWorld};                ///< kWorld: left behind; kLocal: follow the emitter.

    // Spawn
    float rate{0.0F};                   ///< Particles per second while playing.
    std::vector<ParticleBurst> bursts;  ///< Particles emitted at once, in each cycle.
    float duration{1.0F};               ///< Seconds of one cycle.
    bool loop{true};                    ///< true: a new cycle starts after duration; false: plays once.

    // Shape and initial values
    EmitterShape shape;                        ///< Where particles spawn.
    Range lifetime{.min = 1.0F, .max = 1.0F};  ///< Seconds; above 0.
    Range speed{.min = 0.0F, .max = 0.0F};     ///< World units per second, along the angle.
    Range angle{.min = 0.0F,
                .max = 2.0F * std::numbers::pi_v<float>};  ///< Direction, in radians: every way by default.
    Range rotation{.min = 0.0F, .max = 0.0F};              ///< Of the quad, in radians, clockwise on screen.
    Range angularVelocity{.min = 0.0F, .max = 0.0F};       ///< Of the quad, in radians per second.
    Range size{.min = 1.0F, .max = 1.0F};                  ///< Width and height of the quad, in world units.
    Color color{1.0F, 1.0F, 1.0F, 1.0F};                   ///< Multiplied by colorOverLife.

    // Forces
    glm::vec2 gravity{0.0F, 0.0F};  ///< World units per second squared; +Y is down.
    float drag{0.0F};               ///< Slows particles: velocity times exp(-drag * seconds).

    // Over life: 0 at birth, 1 at death
    Gradient colorOverLife;  ///< Multiplies color.
    Curve sizeOverLife;      ///< Multiplies size.
    Curve speedOverLife;     ///< Multiplies the speed.
};

/// @brief Checks @p desc can be simulated: a capacity, finite values, ranges with min <= max, a lifetime above 0,
/// sizes, rate, duration and drag not negative, bursts that happen at least once, and valid curves.
/// @throws exceptions::ParticleException Naming the first field that is invalid.
void validate(const ParticleSystemDesc& desc);

}  // namespace rtype::plugins::backend::graphics
