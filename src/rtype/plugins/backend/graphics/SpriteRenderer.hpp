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
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

// The ECS reflects scalars only: a BlendMode field is reflected as its underlying byte. The specialization must sit
// outside the backend namespace, in one that encloses rtype::ecs, and here rather than in BlendMode.hpp, which the
// renderers include without the ECS.
/// @brief A graphics::BlendMode field is reflected as kU8.
template <>
struct rtype::ecs::FieldTypeOf<rtype::plugins::backend::graphics::BlendMode>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::plugins::backend::graphics::BlendMode) == 1, "BlendMode must match FieldType::kU8");

namespace rtype::plugins::backend {
/// @brief Draws an entity as a part of a texture: the component a render system will turn into a graphics::Sprite.
///
/// @details Only what to draw: where (position, scale, rotation) comes from the entity's GlobalTransform. The texture
/// is referred to by its AssetId, the same in every process, which TextureAssets::load() returns at once; the
/// BackendPlugin's Render system draws the entity once the texture is loaded, sorted by layer. Every field but `source`
/// is reflected (Luau, network).
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {100.0F, 300.0F}})
///     .add(rtype::plugins::backend::SpriteRenderer{
///         .texture = textures->load("assets/ship.png"), .source = {{0.0F, 0.0F}, {32.0F, 16.0F}}, .layer = 10});
/// @endcode
struct SpriteRenderer {
    core::asset::AssetId texture;  ///< The texture to draw, from TextureAssets::load().
    math::Rect source;             ///< Part of the texture to show, in pixels (a frame of a sprite sheet).
    graphics::Color color{1.0F};   ///< Multiplies the texture: tint and transparency.
    glm::vec2 origin{0.0F};        ///< Pivot of the position and the rotation, in pixels from the top-left of source.
    std::int32_t layer{0};         ///< Draw order: higher layers are drawn on top.
    graphics::BlendMode blend{graphics::BlendMode::kAlpha};  ///< How it is combined with what is under it.
};
RTYPE_ECS_COMPONENT(SpriteRenderer, "rtype.SpriteRenderer", texture, color, origin, layer, blend);
}  // namespace rtype::plugins::backend
