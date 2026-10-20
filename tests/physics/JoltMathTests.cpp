/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JoltMathTests
*/

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>
#include <gtest/gtest.h>

#include <array>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include "rtype/physics/detail/JoltMath.hpp"

namespace detail = rtype::physics::detail;

namespace {
constexpr float kTolerance = 1e-6F;
constexpr float kHalfPi = glm::pi<float>() / 2.0F;
}  // namespace

TEST(JoltMath, Vec3KeepsItsComponents) {
    const JPH::Vec3 jolt = detail::toJolt(glm::vec3{1.0F, -2.0F, 3.5F});
    EXPECT_FLOAT_EQ(jolt.GetX(), 1.0F);
    EXPECT_FLOAT_EQ(jolt.GetY(), -2.0F);
    EXPECT_FLOAT_EQ(jolt.GetZ(), 3.5F);
}

TEST(JoltMath, Vec3RoundTripIsExact) {
    const glm::vec3 value{-4.25F, 0.5F, 1e6F};
    EXPECT_EQ(detail::fromJolt(detail::toJolt(value)), value);
}

TEST(JoltMath, QuatKeepsItsComponents) {
    const glm::quat value = glm::quat::wxyz(0.5F, 0.1F, -0.2F, 0.3F);
    const JPH::Quat jolt = detail::toJolt(value);
    EXPECT_FLOAT_EQ(jolt.GetW(), 0.5F);
    EXPECT_FLOAT_EQ(jolt.GetX(), 0.1F);
    EXPECT_FLOAT_EQ(jolt.GetY(), -0.2F);
    EXPECT_FLOAT_EQ(jolt.GetZ(), 0.3F);
}

TEST(JoltMath, QuatRoundTripIsExact) {
    const glm::quat value = glm::quat::wxyz(0.5F, 0.1F, -0.2F, 0.3F);
    EXPECT_EQ(detail::fromJolt(detail::toJolt(value)), value);
}

TEST(JoltMath, QuatRotatesLikeGlm) {
    const glm::quat rotation = glm::angleAxis(kHalfPi, glm::vec3{0.0F, 0.0F, 1.0F});
    const glm::vec3 expected = rotation * glm::vec3{1.0F, 0.0F, 0.0F};
    const glm::vec3 rotated = detail::fromJolt(detail::toJolt(rotation) * JPH::Vec3{1.0F, 0.0F, 0.0F});
    EXPECT_NEAR(rotated.x, expected.x, kTolerance);
    EXPECT_NEAR(rotated.y, expected.y, kTolerance);
    EXPECT_NEAR(rotated.z, expected.z, kTolerance);
}

TEST(JoltMath, ToJolt2DFlipsYAndSetsZToZero) {
    const JPH::Vec3 jolt = detail::toJolt2D(glm::vec2{10.0F, 20.0F});
    EXPECT_FLOAT_EQ(jolt.GetX(), 10.0F);
    EXPECT_FLOAT_EQ(jolt.GetY(), -20.0F);
    EXPECT_FLOAT_EQ(jolt.GetZ(), 0.0F);
}

TEST(JoltMath, FromJolt2DFlipsYAndDropsZ) {
    EXPECT_EQ(detail::fromJolt2D(JPH::Vec3{10.0F, -20.0F, 5.0F}), (glm::vec2{10.0F, 20.0F}));
}

TEST(JoltMath, Vec2RoundTripIsExact) {
    const glm::vec2 value{-123.5F, 456.25F};
    EXPECT_EQ(detail::fromJolt2D(detail::toJolt2D(value)), value);
}

TEST(JoltMath, Angle2DIsNegated) {
    EXPECT_FLOAT_EQ(detail::toJoltAngle2D(1.5F), -1.5F);
    EXPECT_FLOAT_EQ(detail::fromJoltAngle2D(-1.5F), 1.5F);
}

TEST(JoltMath, Rotation2DTurnsClockwiseOnScreen) {
    // A quarter turn clockwise on screen (Y down) takes the X axis to the Y axis, which points down.
    const JPH::Quat rotation = detail::toJoltRotation2D(kHalfPi);
    const glm::vec2 rotated = detail::fromJolt2D(rotation * detail::toJolt2D(glm::vec2{1.0F, 0.0F}));
    EXPECT_NEAR(rotated.x, 0.0F, kTolerance);
    EXPECT_NEAR(rotated.y, 1.0F, kTolerance);
}

TEST(JoltMath, Rotation2DRoundTrip) {
    constexpr std::array kAngles{0.0F, 0.25F, -0.25F, kHalfPi, -kHalfPi, 3.0F, -3.0F};
    for (const float angle : kAngles) {
        EXPECT_NEAR(detail::fromJoltRotation2D(detail::toJoltRotation2D(angle)), angle, kTolerance) << angle;
    }
}
