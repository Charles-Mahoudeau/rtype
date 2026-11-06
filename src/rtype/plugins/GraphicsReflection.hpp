/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsReflection
*/

#pragma once

#include <type_traits>

#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

// Lets the components reflect the renderer contract's graphics types (SpriteRenderer, ShapeRenderer): the ECS maps
// scalars only, so an enum is reflected as its underlying integer and a Rect as the four floats it holds. The
// specializations sit outside the plugins' namespaces, in one that encloses rtype::ecs, and here rather than next to
// the types, which the renderers include without the ECS.

/// @brief A contract::renderer::BlendMode field is reflected as kU8.
template <>
struct rtype::ecs::FieldTypeOf<rtype::contract::renderer::BlendMode>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::contract::renderer::BlendMode) == 1, "BlendMode must match FieldType::kU8");

/// @brief A contract::renderer::math::Rect field is reflected as kVec4: x and y are its position, z and w its width and
/// height.
template <>
struct rtype::ecs::FieldTypeOf<rtype::contract::renderer::math::Rect>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kVec4> {};
static_assert(sizeof(rtype::contract::renderer::math::Rect) == 16 &&
                  alignof(rtype::contract::renderer::math::Rect) == 4,
              "Rect must match FieldType::kVec4");
static_assert(std::is_standard_layout_v<rtype::contract::renderer::math::Rect> &&
                  std::is_trivially_copyable_v<rtype::contract::renderer::math::Rect>,
              "Rect must be four floats in a row: x, y, width, height");
