/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sprites
*/

#include "Sprites.hpp"

#include <raylib.h>

#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>

#include "backend/raylib/renderer/Convert.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"

namespace rtype::backend::raylib::renderer::draw {

void drawSprite(const ::Texture2D& texture, const contract::renderer::Sprite& sprite) {
    ::Rectangle source = sourceOrWhole(sprite.source, texture);
    const glm::vec2 size{source.width, source.height};
    glm::vec2 origin = sprite.origin;
    if (sprite.scale.x < 0.0F) {
        source.width = -source.width;
        origin.x = size.x - origin.x;
    }
    if (sprite.scale.y < 0.0F) {
        source.height = -source.height;
        origin.y = size.y - origin.y;
    }
    const glm::vec2 scale = glm::abs(sprite.scale);
    const ::Rectangle destination{
        .x = sprite.position.x, .y = sprite.position.y, .width = size.x * scale.x, .height = size.y * scale.y};
    DrawTexturePro(texture, source, destination, toVector(origin * scale), sprite.rotation * kDegreesPerRadian,
                   toColor(sprite.color));
}

}  // namespace rtype::backend::raylib::renderer::draw
