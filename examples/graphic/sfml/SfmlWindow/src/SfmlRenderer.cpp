/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SfmlRenderer
*/

#include "SfmlRenderer.hpp"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/geometric.hpp>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>

#include "SfmlPlatform.hpp"
#include "plugins/backend/exceptions/PlatformExceptions.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/View.hpp"
#include "plugins/backend/platform/IPlatform.hpp"

using namespace rtype::plugins::backend;

namespace {

constexpr std::size_t kBytesPerPixel = 4;

/// @return The engine colour (floats in [0, 1]) as an SFML colour (bytes).
sf::Color toSfColor(const graphics::Color& color) {
    constexpr float kMax = 255.0F;
    const auto channel = [](float value) { return static_cast<std::uint8_t>(std::clamp(value, 0.0F, 1.0F) * kMax); };
    return {channel(color.x), channel(color.y), channel(color.z), channel(color.w)};
}

/// @brief Points per half circle of a capsule's outline.
constexpr std::size_t kCapPoints = 12;

/// @return A capsule as a convex polygon: centred on @p center, its straight part @p halfHeight on each side of the
/// center along its axis, turned by @p rotation (radians, clockwise on screen), and @p radius thick.
sf::ConvexShape capsuleShape(glm::vec2 center, float halfHeight, float radius, float rotation) {
    sf::ConvexShape shape{2 * kCapPoints};
    const float pi = std::acos(-1.0F);
    for (std::size_t index = 0; index < kCapPoints; ++index) {
        // The bottom cap (y > 0, Y down) from right to left, then the top cap from left to right.
        const float angle = pi * static_cast<float>(index) / static_cast<float>(kCapPoints - 1);
        shape.setPoint(index, {radius * std::cos(angle), halfHeight + (radius * std::sin(angle))});
        shape.setPoint(kCapPoints + index, {-radius * std::cos(angle), -halfHeight - (radius * std::sin(angle))});
    }
    shape.setPosition({center.x, center.y});
    shape.setRotation(sf::radians(rotation));
    return shape;
}

/// @brief Gives an SFML shape the engine shape's colours; the engine draws the outline inside, SFML with a negative
/// thickness.
void paint(sf::Shape& target, const graphics::Shape& shape) {
    target.setFillColor(toSfColor(shape.fillColor));
    target.setOutlineColor(toSfColor(shape.outlineColor));
    target.setOutlineThickness(-shape.outlineThickness);
}

/// @return A view showing the window in pixels, origin at the top-left corner: the frame's default view.
sf::View pixelView(glm::uvec2 size) {
    return sf::View(sf::FloatRect({0.0F, 0.0F}, {static_cast<float>(size.x), static_cast<float>(size.y)}));
}

/// @brief Turns the distances of an MSDF font atlas into the glyph's coverage, smoothed over about one screen pixel,
/// in the GLSL SFML compiles (its fixed-function inputs: gl_TexCoord, gl_Color).
constexpr const char* kMsdfShader = R"(
uniform sampler2D texture;
uniform float distanceRange;
uniform vec2 textureSize;

float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec3 distances = texture2D(texture, uv).rgb;
    vec2 unitRange = vec2(distanceRange) / textureSize;
    float screenRange = max(0.5 * dot(unitRange, vec2(1.0) / fwidth(uv)), 1.0);
    float coverage = clamp((median(distances.r, distances.g, distances.b) - 0.5) * screenRange + 0.5, 0.0, 1.0);
    gl_FragColor = vec4(gl_Color.rgb, gl_Color.a * coverage);
}
)";

/// @return How SFML draws @p mode. SFML blends straight alpha and cannot drop pixels without a shader, so kCutout is
/// drawn as kAlpha.
sf::RenderStates statesOf(graphics::BlendMode mode) {
    return sf::RenderStates{mode == graphics::BlendMode::kAdditive ? sf::BlendAdd : sf::BlendAlpha};
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

void SfmlRenderer::setView(const graphics::View& view) {
    // Only Orthographic exists: the visit makes the compiler point here the day another projection is added.
    std::visit(
        [&](const graphics::Orthographic& projection) {
            sf::View shown;
            shown.setCenter({view.position.x, view.position.y});
            shown.setSize({static_cast<float>(_framebufferSize.x) / projection.zoom,
                           static_cast<float>(_framebufferSize.y) / projection.zoom});
            shown.setRotation(sf::radians(view.rotation));
            _window->setView(shown);
        },
        view.projection);
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
    _window->draw(sfSprite, statesOf(sprite.blend));
}

void SfmlRenderer::draw(const graphics::Text& text) {
    const sf::Texture* texture = find(text.texture);
    if (texture == nullptr) {
        throw std::invalid_argument("SfmlRenderer::draw: the text's texture was destroyed");
    }
    sf::Shader* shader = msdfShader();
    if (shader == nullptr || text.glyphs.empty()) {
        return;
    }
    const sf::Vector2f textureSize{texture->getSize()};
    shader->setUniform("texture", sf::Shader::CurrentTexture);
    shader->setUniform("distanceRange", text.distanceRange);
    shader->setUniform("textureSize", sf::Glsl::Vec2{textureSize});

    // Two triangles per glyph, its texture coordinates in pixels (SFML's), kept as floats: atlas bounds fall on
    // half pixels.
    constexpr std::size_t kVerticesPerGlyph = 6;
    sf::VertexArray vertices{sf::PrimitiveType::Triangles, text.glyphs.size() * kVerticesPerGlyph};
    const sf::Color color = toSfColor(text.color);
    std::size_t index = 0;
    for (const graphics::GlyphQuad& glyph : text.glyphs) {
        const sf::Vector2f min{glyph.bounds.getX(), glyph.bounds.getY()};
        const sf::Vector2f max = min + sf::Vector2f{glyph.bounds.getWidth(), glyph.bounds.getHeight()};
        const sf::Vector2f uvMin{glyph.source.getX(), glyph.source.getY()};
        const sf::Vector2f uvMax = uvMin + sf::Vector2f{glyph.source.getWidth(), glyph.source.getHeight()};
        const std::array<sf::Vector2f, kVerticesPerGlyph> corners{
            {{0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F}}};
        for (const sf::Vector2f corner : corners) {
            vertices[index++] = sf::Vertex{
                .position = {min.x + ((max.x - min.x) * corner.x), min.y + ((max.y - min.y) * corner.y)},
                .color = color,
                .texCoords = {uvMin.x + ((uvMax.x - uvMin.x) * corner.x), uvMin.y + ((uvMax.y - uvMin.y) * corner.y)}};
        }
    }
    sf::Transformable placement;
    placement.setOrigin({text.origin.x, text.origin.y});
    placement.setPosition({text.position.x, text.position.y});
    placement.setScale({text.scale.x, text.scale.y});
    placement.setRotation(sf::radians(text.rotation));
    sf::RenderStates states = statesOf(text.blend);
    states.transform = placement.getTransform();
    states.texture = texture;
    states.shader = shader;
    _window->draw(vertices, states);
}

void SfmlRenderer::draw(const graphics::Shape& shape) {
    switch (shape.kind) {
        case graphics::ShapeKind::kRect: {
            sf::RectangleShape rect{{shape.size.x, shape.size.y}};
            rect.setOrigin({shape.size.x * 0.5F, shape.size.y * 0.5F});
            rect.setPosition({shape.position.x, shape.position.y});
            rect.setRotation(sf::radians(shape.rotation));
            paint(rect, shape);
            _window->draw(rect, statesOf(shape.blend));
            break;
        }
        case graphics::ShapeKind::kCircle: {
            sf::CircleShape circle{shape.radius};
            circle.setOrigin({shape.radius, shape.radius});
            circle.setPosition({shape.position.x, shape.position.y});
            paint(circle, shape);
            _window->draw(circle, statesOf(shape.blend));
            break;
        }
        case graphics::ShapeKind::kCapsule: {
            sf::ConvexShape capsule = capsuleShape(shape.position, shape.halfHeight, shape.radius, shape.rotation);
            paint(capsule, shape);
            _window->draw(capsule, statesOf(shape.blend));
            break;
        }
        case graphics::ShapeKind::kLine: {
            // A capsule along the segment: its axis, local +Y, turned onto the direction from position to end.
            const glm::vec2 segment = shape.end - shape.position;
            const float rotation = std::atan2(-segment.x, segment.y);
            sf::ConvexShape line =
                capsuleShape((shape.position + shape.end) * 0.5F, glm::length(segment) * 0.5F, shape.radius, rotation);
            paint(line, shape);
            _window->draw(line, statesOf(shape.blend));
            break;
        }
    }
}

sf::Shader* SfmlRenderer::msdfShader() {
    if (!_msdfTried) {
        _msdfTried = true;
        if (sf::Shader::isAvailable()) {
            sf::Shader shader;
            if (shader.loadFromMemory(kMsdfShader, sf::Shader::Type::Fragment)) {
                _msdfShader.emplace(std::move(shader));
            }
        }
    }
    return _msdfShader ? &*_msdfShader : nullptr;
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
