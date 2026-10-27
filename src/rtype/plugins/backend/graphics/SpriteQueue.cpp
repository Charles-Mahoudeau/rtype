/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteQueue
*/

#include "plugins/backend/graphics/SpriteQueue.hpp"

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"

namespace rtype::plugins::backend::graphics {

TextureId SpriteQueue::Item::getTexture() const {
    return std::visit([](const auto& drawn) { return drawn.texture; }, drawing);
}

void SpriteQueue::push(std::int32_t layer, const Sprite& sprite) {
    _items.push_back(Item{.layer = layer, .drawing = sprite});
}

void SpriteQueue::push(std::int32_t layer, Text text) {
    _items.push_back(Item{.layer = layer, .drawing = std::move(text)});
}

std::vector<SpriteQueue::Item> SpriteQueue::take() { return std::exchange(_items, {}); }

}  // namespace rtype::plugins::backend::graphics
