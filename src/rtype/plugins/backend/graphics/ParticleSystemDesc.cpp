/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystemDesc
*/

#include "ParticleSystemDesc.hpp"

#include <cmath>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <variant>

#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/ParticleCurve.hpp"

namespace rtype::plugins::backend::graphics {
namespace {

/// @brief Throws a ParticleException about the field @p field of the desc.
[[noreturn]] void fail(std::string_view field, std::string_view problem) {
    throw exceptions::ParticleException{std::format("ParticleSystemDesc.{} {}", field, problem)};
}

/// @brief Throws if @p value is NaN or infinite, or below @p minimum.
void validateNumber(float value, std::string_view field, float minimum) {
    if (!std::isfinite(value)) {
        fail(field, std::format("is {}, not a finite number", value));
    }
    if (value < minimum) {
        fail(field, std::format("is {}, below {}", value, minimum));
    }
}

/// @brief Throws if a bound of @p range is not finite, below @p minimum, or if min is above max.
void validateRange(const Range& range, std::string_view field, float minimum) {
    validateNumber(range.min, std::format("{}.min", field), minimum);
    validateNumber(range.max, std::format("{}.max", field), minimum);
    if (range.min > range.max) {
        fail(field, std::format("has min = {} above max = {}", range.min, range.max));
    }
}

/// @brief Throws if a burst never happens, or has a time or an interval that is not finite or is negative.
void validateBurst(const ParticleBurst& burst, std::size_t index) {
    const std::string field = std::format("bursts[{}]", index);
    validateNumber(burst.time, std::format("{}.time", field), 0.0F);
    validateNumber(burst.interval, std::format("{}.interval", field), 0.0F);
    if (burst.cycles == 0) {
        fail(field, "has 0 cycles: it never happens");
    }
}

/// @brief Throws if the size of @p shape is not finite or is negative.
void validateShape(const EmitterShape& shape) {
    if (const auto* circle = std::get_if<CircleShape>(&shape)) {
        validateNumber(circle->radius, "shape.radius", 0.0F);
    } else if (const auto* rect = std::get_if<RectShape>(&shape)) {
        validateNumber(rect->size.x, "shape.size.x", 0.0F);
        validateNumber(rect->size.y, "shape.size.y", 0.0F);
    }
}

}  // namespace

void validate(const ParticleSystemDesc& desc) {
    if (desc.capacity == 0) {
        fail("capacity", "is 0: the system could hold no particle");
    }

    validateNumber(desc.rate, "rate", 0.0F);
    for (std::size_t i = 0; i < desc.bursts.size(); ++i) {
        validateBurst(desc.bursts[i], i);
    }
    validateNumber(desc.duration, "duration", 0.0F);
    if (desc.duration == 0.0F) {
        fail("duration", "is 0: a cycle must last");
    }

    validateShape(desc.shape);
    constexpr float kAnyValue = -INFINITY;
    validateRange(desc.lifetime, "lifetime", 0.0F);
    if (desc.lifetime.min == 0.0F) {
        fail("lifetime", "has min = 0: a particle must live");
    }
    validateRange(desc.speed, "speed", kAnyValue);
    validateRange(desc.angle, "angle", kAnyValue);
    validateRange(desc.rotation, "rotation", kAnyValue);
    validateRange(desc.angularVelocity, "angularVelocity", kAnyValue);
    validateRange(desc.size, "size", 0.0F);

    validateNumber(desc.gravity.x, "gravity.x", kAnyValue);
    validateNumber(desc.gravity.y, "gravity.y", kAnyValue);
    validateNumber(desc.drag, "drag", 0.0F);

    try {
        (void)bakeGradient(desc.colorOverLife);
        (void)bakeCurve(desc.sizeOverLife);
        (void)bakeCurve(desc.speedOverLife);
    } catch (const exceptions::ParticleException& error) {
        fail("curves", std::format("are invalid: {}", error.what()));
    }
}

}  // namespace rtype::plugins::backend::graphics
