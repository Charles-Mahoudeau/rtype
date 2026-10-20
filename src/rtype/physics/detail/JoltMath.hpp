/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JoltMath
*/

#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <type_traits>

/// @file
/// @brief Conversions between our GLM types and Jolt's. Internal: only the physics `.cpp` files include it, since it
/// includes Jolt.
///
/// @details 3D values keep their axes. In 2D, our screen is Y down with clockwise angles, while the bodies live in
/// Jolt's XY plane, Y up with counter-clockwise angles around Z: the 2D functions flip Y and negate angles. They also
/// convert linear velocities (same flip) and, with toJoltAngle2D / fromJoltAngle2D, angular velocities around Z.

namespace rtype::physics::detail {

// Positions are converted as JPH::Vec3: true while Jolt is built without double precision.
static_assert(std::is_same_v<JPH::RVec3, JPH::Vec3>, "Jolt's double precision needs RVec3 conversions");

/// @return The same vector in Jolt.
[[nodiscard]] inline JPH::Vec3 toJolt(const glm::vec3& value) noexcept { return {value.x, value.y, value.z}; }

/// @return The same vector in GLM.
[[nodiscard]] inline glm::vec3 fromJolt(JPH::Vec3Arg value) noexcept {
    return {value.GetX(), value.GetY(), value.GetZ()};
}

/// @return The same rotation in Jolt.
[[nodiscard]] inline JPH::Quat toJolt(const glm::quat& value) noexcept {
    return {value.x, value.y, value.z, value.w};  // Jolt takes w last.
}

/// @return The same rotation in GLM.
[[nodiscard]] inline glm::quat fromJolt(JPH::QuatArg value) noexcept {
    return glm::quat::wxyz(value.GetW(), value.GetX(), value.GetY(), value.GetZ());
}

/// @brief Converts a 2D position or linear velocity (Y down) to Jolt's XY plane (Y up).
/// @return (x, -y, 0).
[[nodiscard]] inline JPH::Vec3 toJolt2D(glm::vec2 value) noexcept { return {value.x, -value.y, 0.0F}; }

/// @brief Converts a position or linear velocity in Jolt's XY plane (Y up) to 2D (Y down); Z is dropped.
/// @return (x, -y).
[[nodiscard]] inline glm::vec2 fromJolt2D(JPH::Vec3Arg value) noexcept { return {value.GetX(), -value.GetY()}; }

/// @brief Converts a 2D angle or angular velocity (clockwise on screen) to Jolt's (counter-clockwise around Z).
/// @return The negated value.
[[nodiscard]] constexpr float toJoltAngle2D(float angle) noexcept { return -angle; }

/// @brief Converts an angle or angular velocity around Jolt's Z (counter-clockwise) to 2D (clockwise on screen).
/// @return The negated value.
[[nodiscard]] constexpr float fromJoltAngle2D(float angle) noexcept { return -angle; }

/// @brief Converts a 2D rotation (radians, clockwise on screen) to a Jolt rotation around Z.
[[nodiscard]] inline JPH::Quat toJoltRotation2D(float angle) noexcept {
    return JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), toJoltAngle2D(angle));
}

/// @brief Converts a Jolt rotation around Z to a 2D rotation (radians, clockwise on screen).
/// @return The angle in [-pi, pi]. Any rotation off the Z axis is ignored.
[[nodiscard]] inline float fromJoltRotation2D(JPH::QuatArg value) noexcept {
    return fromJoltAngle2D(value.GetRotationAngle(JPH::Vec3::sAxisZ()));
}

}  // namespace rtype::physics::detail
