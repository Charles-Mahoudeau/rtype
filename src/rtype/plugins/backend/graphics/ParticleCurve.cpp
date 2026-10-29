/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleCurve
*/

#include "ParticleCurve.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <glm/common.hpp>
#include <string_view>
#include <vector>

#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/Color.hpp"

namespace rtype::plugins::backend::graphics {
namespace {

/// @brief Throws if a key's t is outside [0, 1] (NaN included) or comes before the previous key's.
template <typename Key>
void validateKeys(const std::vector<Key>& keys, std::string_view what) {
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const float t = keys[i].t;
        if (std::isnan(t) || t < 0.0F || t > 1.0F) {
            throw exceptions::ParticleException{std::format("{} key {} has t = {}, outside [0, 1]", what, i, t)};
        }
        if (i > 0 && t < keys[i - 1].t) {
            throw exceptions::ParticleException{std::format(
                "{} keys are not sorted by t: key {} (t = {}) comes after t = {}", what, i, t, keys[i - 1].t)};
        }
    }
}

/// @brief Bakes the keys of a Curve or a Gradient: @p fallback all along without keys, the first key's value before
/// it, the last key's after it, and a straight line between the two keys around each sample.
template <typename Key, typename Value>
std::array<Value, kParticleCurveSamples> bakeKeys(const std::vector<Key>& keys, const Value& fallback,
                                                  std::string_view what) {
    validateKeys(keys, what);
    std::array<Value, kParticleCurveSamples> samples{};
    if (keys.empty()) {
        samples.fill(fallback);
        return samples;
    }
    std::size_t from = 0;
    for (std::size_t i = 0; i < kParticleCurveSamples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kParticleCurveSamples - 1);
        if (t <= keys.front().t) {
            samples.at(i) = keys.front().value;
            continue;
        }
        if (t >= keys.back().t) {
            samples.at(i) = keys.back().value;
            continue;
        }
        while (keys.at(from + 1).t <= t) {
            ++from;
        }
        const Key& start = keys.at(from);
        const Key& end = keys.at(from + 1);
        samples.at(i) = glm::mix(start.value, end.value, (t - start.t) / (end.t - start.t));
    }
    return samples;
}

/// @brief Reads a baked table at t, clamped to [0, 1] (NaN reads the first sample).
template <typename Value>
Value sampleTable(const std::array<Value, kParticleCurveSamples>& table, float t) noexcept {
    if (std::isnan(t) || t <= 0.0F) {
        return table.front();
    }
    if (t >= 1.0F) {
        return table.back();
    }
    const float position = t * static_cast<float>(kParticleCurveSamples - 1);
    const auto index = static_cast<std::size_t>(position);
    return glm::mix(table.at(index), table.at(index + 1), position - static_cast<float>(index));
}

}  // namespace

BakedCurve bakeCurve(const Curve& curve) { return bakeKeys(curve.keys, 1.0F, "Curve"); }

BakedGradient bakeGradient(const Gradient& gradient) {
    return bakeKeys(gradient.keys, Color{1.0F, 1.0F, 1.0F, 1.0F}, "Gradient");
}

float sampleBaked(const BakedCurve& curve, float t) noexcept { return sampleTable(curve, t); }

Color sampleBaked(const BakedGradient& gradient, float t) noexcept { return sampleTable(gradient, t); }

}  // namespace rtype::plugins::backend::graphics
