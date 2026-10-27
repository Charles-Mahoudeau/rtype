/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteQueue
*/

#pragma once

#include <cstdint>
#include <vector>

#include "plugins/backend/graphics/Sprite.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief Sprites to draw this frame that are not entities (the tiles of a tilemap, particles), as the ECS resource
/// "rtype.SpriteQueue".
///
/// @details Render systems push sprites whose texture is loaded, after the BackendPlugin's "rtype.backend.camera" and
/// before its "rtype.backend.sprites", which draws them with the SpriteRenderer entities, sorted by layer then by
/// texture, and empties the queue. Among sprites of the same layer and texture, the entities come first, then the
/// queued sprites in the order they were pushed.
/// @code
/// void drawStars(rtype::ecs::ResMut<rtype::plugins::backend::graphics::SpriteQueue> queue) {
///     queue->push(-10, rtype::plugins::backend::graphics::Sprite{.texture = star, .position = {100.0F, 50.0F}});
/// }
/// @endcode
class SpriteQueue {
  public:
    /// @brief One sprite, with the layer it is sorted by.
    struct Item {
        std::int32_t layer{0};  ///< Draw order: higher on top, as SpriteRenderer::layer.
        Sprite sprite;          ///< The draw command; its texture must be loaded.
    };

    SpriteQueue() = default;
    ~SpriteQueue() = default;
    SpriteQueue(const SpriteQueue& other) = default;
    SpriteQueue& operator=(const SpriteQueue& other) = default;
    SpriteQueue(SpriteQueue&& other) noexcept = default;
    SpriteQueue& operator=(SpriteQueue&& other) noexcept = default;

    /// @return The sprites pushed since the last take(), in order.
    [[nodiscard]] const std::vector<Item>& getItems() const noexcept { return _items; }

    /// @brief Adds @p sprite to the frame, sorted by @p layer.
    void push(std::int32_t layer, const Sprite& sprite);

    /// @return The sprites pushed since the last call, in order, leaving the queue empty.
    [[nodiscard]] std::vector<Item> take();

  private:
    std::vector<Item> _items;  ///< In the order they were pushed.
};
RTYPE_ECS_RESOURCE(SpriteQueue, "rtype.SpriteQueue");

}  // namespace rtype::plugins::backend::graphics
