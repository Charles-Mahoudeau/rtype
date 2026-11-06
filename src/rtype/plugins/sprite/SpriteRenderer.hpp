/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteRenderer
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <type_traits>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

// The ECS reflects scalars only: a BlendMode field is reflected as its underlying byte. The specialization must sit
// outside the backend namespace, in one that encloses rtype::ecs, and here rather than in BlendMode.hpp, which the
// renderers include without the ECS.
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

namespace rtype::plugins::sprite {
/// @brief Draws an entity as a part of a texture: the component a render system will turn into a
/// contract::renderer::Sprite.
///
/// @details Only what to draw: where (position, scale, rotation) comes from the entity's GlobalTransform. The texture
/// is referred to by its AssetId, the same in every process, which TextureAssets::load() returns at once; the
/// GraphicsPlugin's Render system draws the entity once the texture is loaded, sorted by layer. Its fields are
/// reflected (Luau, network): `source` as a vec4, its position in x and y, its width and height in z and w.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {100.0F, 300.0F}})
///     .add(rtype::plugins::sprite::SpriteRenderer{
///         .texture = textures->load("assets/ship.png"), .source = {{0.0F, 0.0F}, {32.0F, 16.0F}}, .layer = 10});
/// @endcode
struct SpriteRenderer {
    core::asset::AssetId texture;           ///< The texture to draw, from TextureAssets::load().
    contract::renderer::math::Rect source;  ///< Part of the texture to show, in pixels (a frame of a sprite sheet).
    contract::renderer::Color color{1.0F};  ///< Multiplies the texture: tint and transparency.
    glm::vec2 origin{0.0F};  ///< Pivot of the position and the rotation, in pixels from the top-left of source.
    std::int32_t layer{0};   ///< Draw order: higher layers are drawn on top.
    contract::renderer::BlendMode blend{
        contract::renderer::BlendMode::kAlpha};  ///< How it is combined with what is under it.
    core::asset::AssetId
        material;  ///< Its material, from MaterialAssets::add(); the default one if unset or not ready.
};
RTYPE_ECS_COMPONENT(SpriteRenderer, "rtype.SpriteRenderer", texture, source, color, origin, layer, blend, material);
}  // namespace rtype::plugins::sprite
