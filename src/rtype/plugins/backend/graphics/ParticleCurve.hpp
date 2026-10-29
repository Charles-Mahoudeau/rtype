/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleCurve
*/

#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "plugins/backend/graphics/Color.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief One point of a Curve: its value at a moment of a particle's life.
struct CurveKey {
    float t{0.0F};      ///< 0 at birth, 1 at death.
    float value{1.0F};  ///< The value at t.
};

/// @brief A value over a particle's life (a size or speed multiplier), as keys joined by straight lines: before the
/// first key and after the last, the value stays the one of that key. No key: 1 all along.
struct Curve {
    std::vector<CurveKey> keys;  ///< Sorted by t; two keys at the same t make the value jump.
};

/// @brief One point of a Gradient: its colour at a moment of a particle's life.
struct GradientKey {
    float t{0.0F};                        ///< 0 at birth, 1 at death.
    Color value{1.0F, 1.0F, 1.0F, 1.0F};  ///< The colour at t, alpha included.
};

/// @brief A colour over a particle's life, as keys joined by straight lines, as a Curve. No key: white all along.
struct Gradient {
    std::vector<GradientKey> keys;  ///< Sorted by t; two keys at the same t make the colour jump.
};

/// @brief How many samples a Curve or a Gradient is baked into, evenly spaced from birth (first) to death (last). The
/// particle shaders use the same number.
constexpr std::size_t kParticleCurveSamples = 64;

/// @brief A Curve baked by bakeCurve(), read by sampleBaked().
using BakedCurve = std::array<float, kParticleCurveSamples>;

/// @brief A Gradient baked by bakeGradient(), read by sampleBaked().
using BakedGradient = std::array<Color, kParticleCurveSamples>;

/// @brief Samples @p curve at kParticleCurveSamples evenly spaced moments, once, so that reading it per particle is
/// a lookup. The CPU simulation reads the result with sampleBaked(), the GPU one as a small texture.
/// @throws exceptions::ParticleException If a key's t is outside [0, 1], or the keys are not sorted by t.
[[nodiscard]] BakedCurve bakeCurve(const Curve& curve);

/// @brief Samples @p gradient as bakeCurve() samples a Curve.
/// @throws exceptions::ParticleException If a key's t is outside [0, 1], or the keys are not sorted by t.
[[nodiscard]] BakedGradient bakeGradient(const Gradient& gradient);

/// @return The value of @p curve at @p t (clamped to [0, 1]), linearly interpolated between its two nearest samples,
/// as the GPU's linear filtering does.
[[nodiscard]] float sampleBaked(const BakedCurve& curve, float t) noexcept;

/// @return The colour of @p gradient at @p t (clamped to [0, 1]), as sampleBaked() of a BakedCurve.
[[nodiscard]] Color sampleBaked(const BakedGradient& gradient, float t) noexcept;

}  // namespace rtype::plugins::backend::graphics
