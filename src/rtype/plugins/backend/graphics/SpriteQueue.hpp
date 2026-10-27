/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteQueue
*/

#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief Sprites and texts to draw this frame that are not SpriteRenderer entities (the tiles of a tilemap, the texts
/// of the text plugin, particles), as the ECS resource "rtype.SpriteQueue".
///
/// @details Render systems push sprites and texts whose texture is loaded, after the BackendPlugin's
/// "rtype.backend.camera" and before its "rtype.backend.sprites", which draws them with the SpriteRenderer entities,
/// sorted by layer then by texture, and empties the queue. Among items of the same layer and texture, the entities
/// come first, then the queued items in the order they were pushed.
/// @code
/// void drawStars(rtype::ecs::ResMut<rtype::plugins::backend::graphics::SpriteQueue> queue) {
///     queue->push(-10, rtype::plugins::backend::graphics::Sprite{.texture = star, .position = {100.0F, 50.0F}});
/// }
/// @endcode
class SpriteQueue {
  public:
    /// @brief One sprite or text, with the layer it is sorted by.
    struct Item {
        std::int32_t layer{0};               ///< Draw order: higher on top, as SpriteRenderer::layer.
        std::variant<Sprite, Text> drawing;  ///< The draw command; its texture must be loaded.

        /// @return The texture @p drawing is drawn with, the second sort key.
        [[nodiscard]] TextureId getTexture() const;
    };

    SpriteQueue() = default;
    ~SpriteQueue() = default;
    SpriteQueue(const SpriteQueue& other) = default;
    SpriteQueue& operator=(const SpriteQueue& other) = default;
    SpriteQueue(SpriteQueue&& other) noexcept = default;
    SpriteQueue& operator=(SpriteQueue&& other) noexcept = default;

    /// @return The items pushed since the last take(), in order.
    [[nodiscard]] const std::vector<Item>& getItems() const noexcept { return _items; }

    /// @brief Adds @p sprite to the frame, sorted by @p layer.
    void push(std::int32_t layer, const Sprite& sprite);

    /// @brief Adds @p text to the frame, sorted by @p layer.
    void push(std::int32_t layer, Text text);

    /// @return The items pushed since the last call, in order, leaving the queue empty.
    [[nodiscard]] std::vector<Item> take();

  private:
    std::vector<Item> _items;  ///< In the order they were pushed.
};
RTYPE_ECS_RESOURCE(SpriteQueue, "rtype.SpriteQueue");

}  // namespace rtype::plugins::backend::graphics
