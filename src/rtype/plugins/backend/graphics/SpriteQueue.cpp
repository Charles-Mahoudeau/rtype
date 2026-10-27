/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteQueue
*/

#include "plugins/backend/graphics/SpriteQueue.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include "plugins/backend/graphics/Sprite.hpp"

namespace rtype::plugins::backend::graphics {

void SpriteQueue::push(std::int32_t layer, const Sprite& sprite) {
    _items.push_back(Item{.layer = layer, .sprite = sprite});
}

std::vector<SpriteQueue::Item> SpriteQueue::take() { return std::exchange(_items, {}); }

}  // namespace rtype::plugins::backend::graphics
