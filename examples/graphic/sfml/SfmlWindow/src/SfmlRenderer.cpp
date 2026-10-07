/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SfmlRenderer
*/

#include "SfmlRenderer.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>

#include "SfmlPlatform.hpp"
#include "engine/exceptions/PlatformExceptions.hpp"
#include "engine/graphics/Camera.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/RectShape.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/platform/IPlatform.hpp"

using namespace rtype::engine;

namespace {

constexpr std::size_t kBytesPerPixel = 4;

/// @return The engine colour (floats in [0, 1]) as an SFML colour (bytes).
sf::Color toSfColor(const graphics::Color& color) {
    constexpr float kMax = 255.0F;
    const auto channel = [](float value) { return static_cast<std::uint8_t>(std::clamp(value, 0.0F, 1.0F) * kMax); };
    return {channel(color.x), channel(color.y), channel(color.z), channel(color.w)};
}

/// @return A view showing the window in pixels, origin at the top-left corner: the frame's default camera.
sf::View pixelView(glm::uvec2 size) {
    return sf::View(sf::FloatRect({0.0F, 0.0F}, {static_cast<float>(size.x), static_cast<float>(size.y)}));
}

}  // namespace

namespace example {

void SfmlRenderer::init(platform::IPlatform& platform) {
    auto* sfmlPlatform = dynamic_cast<SfmlPlatform*>(&platform);
    if (sfmlPlatform == nullptr) {
        throw exceptions::UnsupportedFeatureException(
            "SfmlRenderer needs an SfmlPlatform: they share one sf::RenderWindow");
    }
    _window = &sfmlPlatform->getWindow();
    _framebufferSize = platform.getFramebufferSize();
}

void SfmlRenderer::resize(glm::uvec2 framebufferSize) { _framebufferSize = framebufferSize; }

graphics::TextureId SfmlRenderer::createTexture(const graphics::TextureDesc& desc, std::span<const std::byte> pixels) {
    if (pixels.size() != static_cast<std::size_t>(desc.size.x) * desc.size.y * kBytesPerPixel) {
        throw std::invalid_argument("SfmlRenderer::createTexture: pixels must hold size.x * size.y RGBA pixels");
    }
    sf::Texture texture{sf::Vector2u{desc.size.x, desc.size.y}};
    // std::byte and std::uint8_t are both raw bytes: SFML only takes the latter.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    texture.update(reinterpret_cast<const std::uint8_t*>(pixels.data()));
    texture.setSmooth(desc.filter == graphics::TextureFilter::kLinear);
    texture.setRepeated(desc.repeat);

    std::uint32_t index = 0;
    if (_freeSlots.empty()) {
        index = static_cast<std::uint32_t>(_slots.size());
        _slots.emplace_back();
    } else {
        index = _freeSlots.back();
        _freeSlots.pop_back();
    }
    Slot& slot = _slots.at(index);
    slot.texture.emplace(std::move(texture));
    return {.index = index, .generation = slot.generation};
}

void SfmlRenderer::destroy(graphics::TextureId texture) {
    if (find(texture) == nullptr) {
        return;  // Already destroyed: nothing to do.
    }
    Slot& slot = _slots.at(texture.index);
    slot.texture.reset();
    ++slot.generation;  // Every TextureId of the old texture is now stale.
    _freeSlots.push_back(texture.index);
}

void SfmlRenderer::beginFrame(const graphics::Color& clearColor) {
    _window->setView(pixelView(_framebufferSize));
    _window->clear(toSfColor(clearColor));
}

void SfmlRenderer::endFrame() { _window->display(); }

void SfmlRenderer::setCamera(const graphics::Camera& camera) {
    // Only Orthographic exists: the visit makes the compiler point here the day another projection is added.
    std::visit(
        [&](const graphics::Orthographic& projection) {
            sf::View view;
            view.setCenter({camera.position.x, camera.position.y});
            view.setSize({static_cast<float>(_framebufferSize.x) / projection.zoom,
                          static_cast<float>(_framebufferSize.y) / projection.zoom});
            view.setRotation(sf::radians(camera.rotation));
            _window->setView(view);
        },
        camera.projection);
}

void SfmlRenderer::draw(const graphics::Sprite& sprite) {
    const sf::Texture* texture = find(sprite.texture);
    if (texture == nullptr) {
        throw std::invalid_argument("SfmlRenderer::draw: the sprite's texture was destroyed");
    }
    const auto& source = sprite.source;
    sf::Sprite sfSprite{*texture,
                        sf::IntRect({static_cast<int>(source.getX()), static_cast<int>(source.getY())},
                                    {static_cast<int>(source.getWidth()), static_cast<int>(source.getHeight())})};
    sfSprite.setOrigin({sprite.origin.x, sprite.origin.y});
    sfSprite.setPosition({sprite.position.x, sprite.position.y});
    sfSprite.setScale({sprite.scale.x, sprite.scale.y});
    sfSprite.setRotation(sf::radians(sprite.rotation));
    sfSprite.setColor(toSfColor(sprite.color));
    _window->draw(sfSprite);
}

void SfmlRenderer::draw(const graphics::RectShape& rect) {
    sf::RectangleShape shape{{rect.size.x, rect.size.y}};
    shape.setOrigin({rect.origin.x, rect.origin.y});
    shape.setPosition({rect.position.x, rect.position.y});
    shape.setRotation(sf::radians(rect.rotation));
    shape.setFillColor(toSfColor(rect.fillColor));
    shape.setOutlineColor(toSfColor(rect.outlineColor));
    // The engine draws the outline inside the rectangle; SFML does it with a negative thickness.
    shape.setOutlineThickness(-rect.outlineThickness);
    _window->draw(shape);
}

const sf::Texture* SfmlRenderer::find(graphics::TextureId texture) const {
    if (texture.index >= _slots.size()) {
        return nullptr;
    }
    const Slot& slot = _slots.at(texture.index);
    if (slot.generation != texture.generation || !slot.texture) {
        return nullptr;
    }
    return &*slot.texture;
}

}  // namespace example
