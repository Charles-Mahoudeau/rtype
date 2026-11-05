/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformMath
*/

#pragma once

#include "Transform.hpp"

namespace rtype::plugins::transform {
/// @return Where `local`, relative to `parent`, is in the world.
/// @details Exact with a uniform scale or a mirror (a negative scale on one axis, which also reverses the child's
/// rotation). A rotated child of a parent scaled differently on each axis would need a shear, which GlobalTransform
/// cannot hold: its position stays right, its rotation and scale are approximate. TODO
[[nodiscard]] GlobalTransform combine(const GlobalTransform& parent, const Transform& local) noexcept;

/// @return `world` expressed relative to `parent`: the inverse of combine().
/// @details A parent scaled to 0 on an axis loses that axis: the result is 0 on it, not a division by 0.
[[nodiscard]] Transform toLocal(const GlobalTransform& parent, const GlobalTransform& world) noexcept;
}  // namespace rtype::plugins::transform
