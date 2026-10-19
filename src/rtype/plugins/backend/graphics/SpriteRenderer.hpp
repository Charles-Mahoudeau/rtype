/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteRenderer
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::backend {
/// @brief Draws an entity as a part of a texture: the component a render system will turn into a graphics::Sprite.
///
/// @details Only what to draw: where (position, scale, rotation) will come from a Transform component. The texture is
/// referred to by id, never owned: it lives in the renderer and is recorded in the TextureAssets resource. Its fields
/// are not reflected: a TextureId is local to one renderer, so it must not reach Luau's saves or the network.
/// @code
/// world.getComponents().add(ship, rtype::plugins::backend::SpriteRenderer{
///     .texture = shipTexture, .source = {{0.0F, 0.0F}, {32.0F, 16.0F}}, .layer = 10});
/// @endcode
struct SpriteRenderer {
    graphics::TextureId texture;  ///< The texture to draw; not owned.
    math::Rect source;            ///< Part of the texture to show, in pixels (a frame of a sprite sheet).
    graphics::Color color{1.0F};  ///< Multiplies the texture: tint and transparency.
    glm::vec2 origin{0.0F};       ///< Pivot of the position and the rotation, in pixels from the top-left of source.
    std::int32_t layer{0};        ///< Draw order: higher layers are drawn on top.
};
RTYPE_ECS_COMPONENT(SpriteRenderer, "rtype.SpriteRenderer");
}  // namespace rtype::plugins::backend
