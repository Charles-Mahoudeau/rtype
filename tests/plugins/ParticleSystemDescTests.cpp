/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystemDescTests
*/

#include <gtest/gtest.h>

#include <limits>

#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/ParticleCurve.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"

namespace {
using rtype::plugins::backend::exceptions::ParticleException;
using rtype::plugins::backend::graphics::CircleShape;
using rtype::plugins::backend::graphics::Curve;
using rtype::plugins::backend::graphics::ParticleBurst;
using rtype::plugins::backend::graphics::ParticleSystemDesc;
using rtype::plugins::backend::graphics::Range;
using rtype::plugins::backend::graphics::RectShape;
using rtype::plugins::backend::graphics::validate;

constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kInfinity = std::numeric_limits<float>::infinity();
}  // namespace

TEST(ParticleSystemDesc, DefaultIsValid) { EXPECT_NO_THROW(validate(ParticleSystemDesc{})); }

TEST(ParticleSystemDesc, AFullEffectIsValid) {
    const ParticleSystemDesc desc{
        .capacity = 4096,
        .rate = 200.0F,
        .bursts = {ParticleBurst{.time = 0.5F, .count = 50, .cycles = 3, .interval = 0.25F}},
        .duration = 2.0F,
        .shape = CircleShape{.radius = 16.0F, .edgeOnly = true},
        .lifetime = {.min = 0.5F, .max = 1.5F},
        .speed = {.min = -10.0F, .max = 80.0F},
        .size = {.min = 0.0F, .max = 4.0F},
        .gravity = {0.0F, 98.0F},
        .drag = 0.5F,
        .sizeOverLife = Curve{.keys = {{.t = 0.0F, .value = 1.0F}, {.t = 1.0F, .value = 0.0F}}},
    };
    EXPECT_NO_THROW(validate(desc));
}

TEST(ParticleSystemDesc, RejectsZeroCapacity) {
    EXPECT_THROW(validate(ParticleSystemDesc{.capacity = 0}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsANegativeOrNaNRate) {
    EXPECT_THROW(validate(ParticleSystemDesc{.rate = -1.0F}), ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.rate = kNaN}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsAZeroOrInfiniteDuration) {
    EXPECT_THROW(validate(ParticleSystemDesc{.duration = 0.0F}), ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.duration = kInfinity}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsInvalidBursts) {
    EXPECT_THROW(validate(ParticleSystemDesc{.bursts = {ParticleBurst{.count = 10, .cycles = 0}}}), ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.bursts = {ParticleBurst{.time = -1.0F, .count = 10}}}),
                 ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.bursts = {ParticleBurst{.count = 10, .cycles = 2, .interval = kNaN}}}),
                 ParticleException);
}

TEST(ParticleSystemDesc, RejectsANegativeShape) {
    EXPECT_THROW(validate(ParticleSystemDesc{.shape = CircleShape{.radius = -1.0F}}), ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.shape = RectShape{.size = {10.0F, -1.0F}}}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsARangeWithMinAboveMax) {
    EXPECT_THROW(validate(ParticleSystemDesc{.speed = Range{.min = 2.0F, .max = 1.0F}}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsANaNInARange) {
    EXPECT_THROW(validate(ParticleSystemDesc{.angle = Range{.min = kNaN, .max = 1.0F}}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsALifetimeThatIsNotAboveZero) {
    EXPECT_THROW(validate(ParticleSystemDesc{.lifetime = Range{.min = 0.0F, .max = 1.0F}}), ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.lifetime = Range{.min = -1.0F, .max = 1.0F}}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsANegativeSize) {
    EXPECT_THROW(validate(ParticleSystemDesc{.size = Range{.min = -1.0F, .max = 1.0F}}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsANegativeDragOrANaNGravity) {
    EXPECT_THROW(validate(ParticleSystemDesc{.gravity = {kNaN, 0.0F}}), ParticleException);
    EXPECT_THROW(validate(ParticleSystemDesc{.drag = -0.1F}), ParticleException);
}

TEST(ParticleSystemDesc, RejectsInvalidCurves) {
    const Curve unsorted{.keys = {{.t = 1.0F, .value = 0.0F}, {.t = 0.0F, .value = 1.0F}}};
    EXPECT_THROW(validate(ParticleSystemDesc{.speedOverLife = unsorted}), ParticleException);
}
