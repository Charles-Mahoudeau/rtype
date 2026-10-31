/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleRandomTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>

#include "plugins/backend/graphics/ParticleRandom.hpp"

namespace {
using rtype::plugins::backend::graphics::particleRandom;
using rtype::plugins::backend::graphics::particleRandomFloat;
using rtype::plugins::backend::graphics::ParticleRandomStream;
using rtype::plugins::backend::graphics::pcgHash;

constexpr std::uint32_t kDraws = 10000;

constexpr std::array kStreams{ParticleRandomStream::kLifetime,        ParticleRandomStream::kSpeed,
                              ParticleRandomStream::kAngle,           ParticleRandomStream::kRotation,
                              ParticleRandomStream::kAngularVelocity, ParticleRandomStream::kSize,
                              ParticleRandomStream::kShapeX,          ParticleRandomStream::kShapeY};

// pcgHash() is constexpr: its reference values are checked when compiling too.
static_assert(pcgHash(0U) == 129708002U);
static_assert(particleRandomFloat(0U, 0U, ParticleRandomStream::kLifetime) >= 0.0F);
static_assert(particleRandomFloat(0U, 0U, ParticleRandomStream::kLifetime) < 1.0F);
}  // namespace

// The reference values of the PCG hash, computed by an independent implementation: the GLSL version must give the same.
TEST(ParticleRandom, PcgHashMatchesItsReferenceValues) {
    EXPECT_EQ(pcgHash(0U), 129708002U);
    EXPECT_EQ(pcgHash(1U), 2831084092U);
    EXPECT_EQ(pcgHash(42U), 1223963391U);
    EXPECT_EQ(pcgHash(0xFFFFFFFFU), 3861530882U);
}

TEST(ParticleRandom, SameInputsGiveTheSameNumber) {
    EXPECT_EQ(particleRandom(7U, 42U, ParticleRandomStream::kSpeed),
              particleRandom(7U, 42U, ParticleRandomStream::kSpeed));
}

TEST(ParticleRandom, EachStreamGivesItsOwnNumber) {
    std::set<std::uint32_t> numbers;
    for (const ParticleRandomStream stream : kStreams) {
        numbers.insert(particleRandom(7U, 42U, stream));
    }
    EXPECT_EQ(numbers.size(), kStreams.size());
}

TEST(ParticleRandom, SeedAndParticleAreNotInterchangeable) {
    EXPECT_NE(particleRandom(1U, 2U, ParticleRandomStream::kLifetime),
              particleRandom(2U, 1U, ParticleRandomStream::kLifetime));
}

TEST(ParticleRandom, NeighbouringParticlesGetDifferentNumbers) {
    std::set<std::uint32_t> numbers;
    for (std::uint32_t particle = 0; particle < kDraws; ++particle) {
        numbers.insert(particleRandom(7U, particle, ParticleRandomStream::kAngle));
    }
    // A 32-bit hash of 10000 inputs may collide, but only rarely.
    EXPECT_GT(numbers.size(), kDraws - 5);
}

TEST(ParticleRandom, FloatsStayInZeroToOne) {
    for (std::uint32_t particle = 0; particle < kDraws; ++particle) {
        const float value = particleRandomFloat(7U, particle, ParticleRandomStream::kSize);
        EXPECT_GE(value, 0.0F);
        EXPECT_LT(value, 1.0F);
    }
}

TEST(ParticleRandom, FloatsAreRoughlyUniform) {
    constexpr std::size_t kBuckets = 10;
    std::array<std::uint32_t, kBuckets> counts{};
    double sum = 0.0;
    for (std::uint32_t particle = 0; particle < kDraws; ++particle) {
        const float value = particleRandomFloat(7U, particle, ParticleRandomStream::kShapeX);
        sum += value;
        ++counts.at(static_cast<std::size_t>(value * static_cast<float>(kBuckets)));
    }
    EXPECT_NEAR(sum / kDraws, 0.5, 0.02);
    // 1000 expected per bucket: a uniform draw stays well within 15% of it.
    for (const std::uint32_t count : counts) {
        EXPECT_NEAR(static_cast<double>(count), kDraws / static_cast<double>(kBuckets), 150.0);
    }
}

TEST(ParticleRandom, StreamsOfOneParticleAreNotCorrelated) {
    // If two streams were correlated, a particle with a long lifetime would also tend to be fast.
    double sumLifetime = 0.0;
    double sumSpeed = 0.0;
    double sumProduct = 0.0;
    for (std::uint32_t particle = 0; particle < kDraws; ++particle) {
        const double lifetime = particleRandomFloat(7U, particle, ParticleRandomStream::kLifetime);
        const double speed = particleRandomFloat(7U, particle, ParticleRandomStream::kSpeed);
        sumLifetime += lifetime;
        sumSpeed += speed;
        sumProduct += lifetime * speed;
    }
    const double covariance = (sumProduct / kDraws) - ((sumLifetime / kDraws) * (sumSpeed / kDraws));
    // Two independent uniform variables in [0, 1) have a variance of 1/12 each: a covariance near 0 means no link.
    EXPECT_NEAR(covariance / (1.0 / 12.0), 0.0, 0.05);
}
