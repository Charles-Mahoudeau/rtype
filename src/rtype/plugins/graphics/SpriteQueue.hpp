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

#include "plugins/graphics/Export.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::graphics {

/// @brief The sprites, texts and particles to draw this frame (the sprite entities, the tiles of a tilemap, the texts
/// of the text plugin, the emitters of the particle plugin), as the ECS resource "rtype.SpriteQueue".
///
/// @details Render systems push what they draw, after the GraphicsPlugin's "rtype.graphics.camera" and before its
/// "rtype.graphics.sprites", which draws it sorted by layer then by texture, and empties the queue. They take it as a
/// ResAppend, so they need no order between them: among items of the same layer and texture, the order is the one the
/// systems ran in.
/// @code
/// void drawStars(rtype::ecs::ResAppend<rtype::plugins::graphics::SpriteQueue> queue) {
///     queue->push(-10, rtype::contract::renderer::Sprite{.texture = star, .position = {100.0F, 50.0F}});
/// }
/// @endcode
class RTYPE_PLUGIN_GRAPHICS_API SpriteQueue {
  public:
    /// @brief One sprite, text or emitter's particles, with the layer it is sorted by.
    struct RTYPE_PLUGIN_GRAPHICS_API Item {
        std::int32_t layer{0};  ///< Draw order: higher on top, as SpriteRenderer::layer.
        std::variant<contract::renderer::Sprite, contract::renderer::Text, contract::renderer::ParticleDraw>
            drawing;  ///< The draw command; its texture must be loaded.

        /// @return The texture @p drawing is drawn with, the second sort key.
        [[nodiscard]] contract::renderer::TextureId getTexture() const;
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
    void push(std::int32_t layer, const contract::renderer::Sprite& sprite);

    /// @brief Adds @p text to the frame, sorted by @p layer.
    void push(std::int32_t layer, contract::renderer::Text text);

    /// @brief Adds the particles of one emitter to the frame, sorted by @p layer. CPU instances are not copied: they
    /// must stay valid until the GraphicsPlugin's "rtype.graphics.sprites" has drawn the queue.
    void push(std::int32_t layer, const contract::renderer::ParticleDraw& particles);

    /// @return The items pushed since the last call, in order, leaving the queue empty.
    [[nodiscard]] std::vector<Item> take();

  private:
    std::vector<Item> _items;  ///< In the order they were pushed.
};
RTYPE_ECS_RESOURCE(SpriteQueue, "rtype.SpriteQueue");

}  // namespace rtype::plugins::graphics
