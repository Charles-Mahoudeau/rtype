/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SfmlRenderer
*/

#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <vector>

#include "engine/graphics/Camera.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/graphics/RectShape.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/platform/IPlatform.hpp"

namespace example {

/// @brief A complete IRenderer written with SFML 3, drawing into the sf::RenderWindow of SfmlPlatform.
///
/// @details Shows the other way to pair a renderer with its platform: no surface, no loader, the renderer simply
/// shares the platform's window. init() gets it with a dynamic_cast, so pairing it with another platform throws
/// UnsupportedFeatureException.
///
/// Textures live in a table of slots: a TextureId is a slot index plus the slot's generation, bumped each time the
/// slot is freed, so a TextureId used after destroy() is detected instead of reaching the next texture of the slot.
class SfmlRenderer final : public rtype::engine::graphics::IRenderer {
  public:
    SfmlRenderer() = default;
    ~SfmlRenderer() override = default;
    SfmlRenderer(const SfmlRenderer&) = delete;
    SfmlRenderer& operator=(const SfmlRenderer&) = delete;
    SfmlRenderer(SfmlRenderer&&) = delete;
    SfmlRenderer& operator=(SfmlRenderer&&) = delete;

    /// @throws rtype::engine::exceptions::UnsupportedFeatureException If the platform is not an SfmlPlatform.
    void init(rtype::engine::platform::IPlatform& platform) override;
    void resize(glm::uvec2 framebufferSize) override;

    /// @throws std::invalid_argument If pixels does not hold size.x * size.y RGBA pixels.
    [[nodiscard]] rtype::engine::graphics::TextureId createTexture(const rtype::engine::graphics::TextureDesc& desc,
                                                                   std::span<const std::byte> pixels) override;
    void destroy(rtype::engine::graphics::TextureId texture) override;

    /// @brief Clears the window, and resets the camera to window pixels (origin at the top-left corner).
    void beginFrame(const rtype::engine::graphics::Color& clearColor) override;
    void endFrame() override;

    void setCamera(const rtype::engine::graphics::Camera& camera) override;
    /// @throws std::invalid_argument If the sprite's texture was destroyed.
    void draw(const rtype::engine::graphics::Sprite& sprite) override;
    void draw(const rtype::engine::graphics::RectShape& rect) override;

  private:
    /// @brief A place for one texture; reused after destroy() with a higher generation.
    struct Slot {
        std::uint32_t generation = 1;        ///< Generation of the TextureId that refers to the current texture.
        std::optional<sf::Texture> texture;  ///< Empty while the slot is free.
    };

    /// @return The texture a TextureId refers to, or null if it was destroyed (or never existed).
    [[nodiscard]] const sf::Texture* find(rtype::engine::graphics::TextureId texture) const;

    sf::RenderWindow* _window{nullptr};     ///< The platform's window, set by init().
    glm::uvec2 _framebufferSize{0};         ///< Window size in pixels, for the default view and the camera.
    std::vector<Slot> _slots;               ///< Every texture slot, indexed by TextureId::index.
    std::vector<std::uint32_t> _freeSlots;  ///< Indices of the free slots, reused first.
};
}  // namespace example
