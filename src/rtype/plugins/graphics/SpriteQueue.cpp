/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteQueue
*/

#include "plugins/graphics/SpriteQueue.hpp"

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"

namespace rtype::plugins::graphics {

contract::renderer::TextureId SpriteQueue::Item::getTexture() const {
    return std::visit([](const auto& drawn) { return drawn.texture; }, drawing);
}

void SpriteQueue::push(std::int32_t layer, const contract::renderer::Sprite& sprite) {
    _items.push_back(Item{.layer = layer, .drawing = sprite});
}

void SpriteQueue::push(std::int32_t layer, contract::renderer::Text text) {
    _items.push_back(Item{.layer = layer, .drawing = std::move(text)});
}

void SpriteQueue::push(std::int32_t layer, const contract::renderer::ParticleDraw& particles) {
    _items.push_back(Item{.layer = layer, .drawing = particles});
}

std::vector<SpriteQueue::Item> SpriteQueue::take() { return std::exchange(_items, {}); }

}  // namespace rtype::plugins::graphics
