/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleCurveTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>

#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/ParticleCurve.hpp"

namespace {
using rtype::plugins::backend::exceptions::ParticleException;
using rtype::plugins::backend::graphics::bakeCurve;
using rtype::plugins::backend::graphics::BakedCurve;
using rtype::plugins::backend::graphics::bakeGradient;
using rtype::plugins::backend::graphics::Color;
using rtype::plugins::backend::graphics::Curve;
using rtype::plugins::backend::graphics::Gradient;
using rtype::plugins::backend::graphics::kParticleCurveSamples;
using rtype::plugins::backend::graphics::sampleBaked;

constexpr float kTolerance = 1e-5F;

/// @return The moment of sample @p index of a baked table.
float sampleTime(std::size_t index) {
    return static_cast<float>(index) / static_cast<float>(kParticleCurveSamples - 1);
}

/// @brief Checks every sample of @p baked is a number, not NaN or infinite.
void expectFinite(const BakedCurve& baked) {
    for (const float sample : baked) {
        EXPECT_TRUE(std::isfinite(sample));
    }
}
}  // namespace

TEST(ParticleCurve, NoKeyIsOneAllAlong) {
    for (const float sample : bakeCurve(Curve{})) {
        EXPECT_FLOAT_EQ(sample, 1.0F);
    }
}

TEST(ParticleCurve, OneKeyHoldsItsValueAllAlong) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.5F, .value = 2.0F}}});
    expectFinite(baked);
    for (const float sample : baked) {
        EXPECT_FLOAT_EQ(sample, 2.0F);
    }
}

TEST(ParticleCurve, InterpolatesBetweenTwoKeys) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.0F, .value = 0.0F}, {.t = 1.0F, .value = 1.0F}}});
    for (std::size_t i = 0; i < kParticleCurveSamples; ++i) {
        EXPECT_NEAR(baked.at(i), sampleTime(i), kTolerance);
    }
}

TEST(ParticleCurve, HoldsTheFirstValueBeforeTheFirstKey) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.5F, .value = 1.0F}, {.t = 1.0F, .value = 0.0F}}});
    EXPECT_FLOAT_EQ(baked.front(), 1.0F);
    EXPECT_FLOAT_EQ(baked.at(kParticleCurveSamples / 4), 1.0F);
}

TEST(ParticleCurve, HoldsTheLastValueAfterTheLastKey) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.0F, .value = 1.0F}, {.t = 0.5F, .value = 0.0F}}});
    expectFinite(baked);
    EXPECT_FLOAT_EQ(baked.back(), 0.0F);
    EXPECT_FLOAT_EQ(baked.at(kParticleCurveSamples * 3 / 4), 0.0F);
}

TEST(ParticleCurve, TwoKeysAtTheSameTimeJump) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.0F, .value = 0.0F},
                                                      {.t = 0.5F, .value = 0.0F},
                                                      {.t = 0.5F, .value = 1.0F},
                                                      {.t = 1.0F, .value = 1.0F}}});
    expectFinite(baked);
    for (std::size_t i = 0; i < kParticleCurveSamples; ++i) {
        EXPECT_FLOAT_EQ(baked.at(i), sampleTime(i) < 0.5F ? 0.0F : 1.0F);
    }
}

TEST(ParticleCurve, ThrowsOnKeysNotSortedByTime) {
    const Curve curve{.keys = {{.t = 1.0F, .value = 0.0F}, {.t = 0.0F, .value = 1.0F}}};
    EXPECT_THROW((void)bakeCurve(curve), ParticleException);
}

TEST(ParticleCurve, ThrowsOnTimeOutsideZeroToOne) {
    EXPECT_THROW((void)bakeCurve(Curve{.keys = {{.t = 1.5F, .value = 0.0F}}}), ParticleException);
    EXPECT_THROW((void)bakeCurve(Curve{.keys = {{.t = -0.1F, .value = 0.0F}}}), ParticleException);
    EXPECT_THROW((void)bakeCurve(Curve{.keys = {{.t = std::numeric_limits<float>::quiet_NaN(), .value = 0.0F}}}),
                 ParticleException);
}

TEST(ParticleGradient, NoKeyIsWhiteAllAlong) {
    for (const Color& sample : bakeGradient(Gradient{})) {
        EXPECT_EQ(sample, (Color{1.0F, 1.0F, 1.0F, 1.0F}));
    }
}

TEST(ParticleGradient, InterpolatesEveryChannel) {
    const Gradient gradient{
        .keys = {{.t = 0.0F, .value = {1.0F, 0.0F, 0.0F, 1.0F}}, {.t = 1.0F, .value = {0.0F, 0.0F, 1.0F, 0.0F}}}};
    const Color middle = sampleBaked(bakeGradient(gradient), 0.5F);
    EXPECT_NEAR(middle.x, 0.5F, kTolerance);
    EXPECT_NEAR(middle.y, 0.0F, kTolerance);
    EXPECT_NEAR(middle.z, 0.5F, kTolerance);
    EXPECT_NEAR(middle.w, 0.5F, kTolerance);
}

TEST(ParticleGradient, ThrowsOnKeysNotSortedByTime) {
    const Gradient gradient{.keys = {{.t = 0.8F}, {.t = 0.2F}}};
    EXPECT_THROW((void)bakeGradient(gradient), ParticleException);
}

TEST(ParticleCurveSampling, ReadsTheEndsAndInterpolatesBetweenSamples) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.0F, .value = 0.0F}, {.t = 1.0F, .value = 10.0F}}});
    EXPECT_FLOAT_EQ(sampleBaked(baked, 0.0F), 0.0F);
    EXPECT_FLOAT_EQ(sampleBaked(baked, 1.0F), 10.0F);
    EXPECT_NEAR(sampleBaked(baked, 0.37F), 3.7F, 1e-4F);
}

TEST(ParticleCurveSampling, ClampsTimeOutsideZeroToOne) {
    const BakedCurve baked = bakeCurve(Curve{.keys = {{.t = 0.0F, .value = 2.0F}, {.t = 1.0F, .value = 4.0F}}});
    EXPECT_FLOAT_EQ(sampleBaked(baked, -1.0F), 2.0F);
    EXPECT_FLOAT_EQ(sampleBaked(baked, 2.0F), 4.0F);
    EXPECT_FLOAT_EQ(sampleBaked(baked, std::numeric_limits<float>::quiet_NaN()), 2.0F);
}
