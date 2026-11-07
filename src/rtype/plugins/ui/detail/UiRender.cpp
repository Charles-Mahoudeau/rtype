/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiRender
*/

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/graphics/OverlayQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/ui/UiComponents.hpp"
#include "plugins/ui/UiNode.hpp"
#include "plugins/ui/UiWidgets.hpp"
#include "plugins/ui/detail/UiLayout.hpp"
#include "plugins/ui/detail/UiSystems.hpp"
#include "plugins/ui/detail/UiText.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Overlay.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::ui::detail {
namespace {
using contract::renderer::Color;
using contract::renderer::OverlayItem;
using contract::renderer::Shape;
using contract::renderer::ShapeKind;
using contract::renderer::Sprite;
using contract::renderer::math::Rect;

/// @return The size @p shown takes, wrapped to @p available if it asks and that is not negative.
[[nodiscard]] glm::vec2 measure(const text::Font& font, const UiText& shown, const float available) {
    // An empty text still takes a line, so an empty input keeps its height.
    const std::string& content = shown.text.empty() ? std::string{" "} : shown.text;
    const glm::vec2 line = text::measureText(font, "Ag", shown.size);
    glm::vec2 size = shown.wrap && available >= 0.0F
                         ? text::measureText(font, wrapText(font, content, shown.size, available), shown.size)
                         : text::measureText(font, content, shown.size);
    size.y = std::max(size.y, line.y);
    return size;
}

/// @brief Adds what a frame draws, every item clipped by the node it belongs to.
class Painter {
  public:
    Painter(std::vector<OverlayItem>& items, const glm::vec4& clip) noexcept
        : _items{&items}, _clip{Rect{{clip.x, clip.y}, {clip.z, clip.w}}} {}
    ~Painter() = default;
    Painter(const Painter& other) = delete;
    Painter& operator=(const Painter& other) = delete;
    Painter(Painter&& other) = delete;
    Painter& operator=(Painter&& other) = delete;

    /// @brief Adds a rectangle of @p size at @p position, its top-left corner.
    void rectangle(const glm::vec2 position, const glm::vec2 size, const float radius, const Color& color,
                   const Color& outline = Color{0.0F}, const float thickness = 0.0F) {
        _items->push_back(OverlayItem{.drawing = Shape{.kind = ShapeKind::kRect,
                                                       .position = position + (size * 0.5F),
                                                       .size = size,
                                                       .cornerRadius = radius,
                                                       .fillColor = color,
                                                       .outlineColor = outline,
                                                       .outlineThickness = thickness},
                                      .clip = _clip});
    }

    /// @brief Adds a disc of @p radius around @p center.
    void circle(const glm::vec2 center, const float radius, const Color& color) {
        _items->push_back(OverlayItem{
            .drawing = Shape{.kind = ShapeKind::kCircle, .position = center, .radius = radius, .fillColor = color},
            .clip = _clip});
    }

    /// @brief Adds the part @p source of @p texture, stretched over @p size at @p position.
    void image(const contract::renderer::TextureId texture, const Rect& source, const glm::vec2 position,
               const glm::vec2 size, const Color& color) {
        if (source.getWidth() <= 0.0F || source.getHeight() <= 0.0F || size.x <= 0.0F || size.y <= 0.0F) {
            return;
        }
        _items->push_back(
            OverlayItem{.drawing = Sprite{.texture = texture,
                                          .source = source,
                                          .position = position,
                                          .scale = size / glm::vec2{source.getWidth(), source.getHeight()},
                                          .color = color},
                        .clip = _clip});
    }

    /// @brief Adds @p drawn.
    void text(contract::renderer::Text drawn) {
        _items->push_back(OverlayItem{.drawing = std::move(drawn), .clip = _clip});
    }

  private:
    std::vector<OverlayItem>* _items;  ///< What the frame draws.
    std::optional<Rect> _clip;         ///< What the node is visible in.
};

/// @brief Draws @p image over @p node, by nine parts if it has a slice.
void paintImage(Painter& painter, const graphics::TextureAssets& textures, const UiComputed& node,
                const UiImage& image) {
    const std::optional<contract::renderer::TextureId> texture = textures.getTexture(image.texture);
    const graphics::TextureAssets::Entry* entry = textures.get(image.texture);
    if (!texture || entry == nullptr) {
        return;
    }
    const glm::vec2 whole{entry->desc.size};
    const Rect source = image.source.getWidth() > 0.0F ? image.source : Rect{{0.0F, 0.0F}, whole};
    if (image.slice == glm::vec4{0.0F}) {
        painter.image(*texture, source, node.position, node.size, image.color);
        return;
    }
    // Three columns and three rows, in the source and on the node: the borders keep their size.
    const glm::vec2 start{source.getX(), source.getY()};
    const glm::vec2 end = start + glm::vec2{source.getWidth(), source.getHeight()};
    const glm::vec2 first{image.slice.x, image.slice.y};
    const glm::vec2 last{image.slice.z, image.slice.w};
    const std::array<glm::vec2, 4> from{start, start + first, end - last, end};
    const glm::vec2 scaledFirst = first * image.sliceScale;
    const glm::vec2 scaledLast = last * image.sliceScale;
    const std::array<glm::vec2, 4> to{node.position, node.position + scaledFirst,
                                      node.position + node.size - scaledLast, node.position + node.size};
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            const glm::vec2 sourceStart{from.at(column).x, from.at(row).y};
            const glm::vec2 sourceEnd{from.at(column + 1).x, from.at(row + 1).y};
            const glm::vec2 targetStart{to.at(column).x, to.at(row).y};
            const glm::vec2 targetEnd{to.at(column + 1).x, to.at(row + 1).y};
            painter.image(*texture, Rect{sourceStart, sourceEnd - sourceStart}, targetStart, targetEnd - targetStart,
                          image.color);
        }
    }
}

/// @return The top-left corner and the width of the content of @p node.
[[nodiscard]] std::pair<glm::vec2, float> contentOf(const UiComputed& node) noexcept {
    return {node.position + glm::vec2{node.padding.x, node.padding.y}, node.size.x - node.padding.x - node.padding.z};
}

/// @brief Draws @p content in the style of @p shown, in @p color, inside @p node.
void paintText(Painter& painter, const text::FontAssets& fonts, const graphics::TextureAssets& textures,
               const UiComputed& node, const UiText& shown, const std::string& content, const Color& color) {
    const text::Font* font = fonts.get(shown.font);
    if (font == nullptr || content.empty()) {
        return;
    }
    const std::optional<contract::renderer::TextureId> texture =
        textures.getTexture(core::asset::AssetId::fromPath(font->getTexturePath()));
    if (!texture) {
        return;
    }
    const auto [start, width] = contentOf(node);
    text::TextLayout layout = text::layoutText(
        *font, shown.wrap ? wrapText(*font, content, shown.size, width) : content, shown.size, shown.align);
    if (layout.glyphs.empty()) {
        return;
    }
    float offset = 0.0F;
    if (shown.align == text::TextAlign::kCenter) {
        offset = (width - layout.size.x) * 0.5F;
    } else if (shown.align == text::TextAlign::kRight) {
        offset = width - layout.size.x;
    }
    painter.text(contract::renderer::Text{.texture = *texture,
                                          .glyphs = std::move(layout.glyphs),
                                          .position = start + glm::vec2{offset, 0.0F},
                                          .color = color,
                                          .distanceRange = font->getDistanceRange()});
}

/// @brief Draws a slider across @p node.
void paintSlider(Painter& painter, const UiComputed& node, const UiSlider& slider) {
    const float range = slider.max - slider.min;
    const float ratio = range != 0.0F ? std::clamp((slider.value - slider.min) / range, 0.0F, 1.0F) : 0.0F;
    const float middle = node.position.y + (node.size.y * 0.5F);
    const glm::vec2 start{node.position.x, middle - (slider.trackHeight * 0.5F)};
    const float radius = slider.trackHeight * 0.5F;
    painter.rectangle(start, {node.size.x, slider.trackHeight}, radius, slider.trackColor);
    painter.rectangle(start, {node.size.x * ratio, slider.trackHeight}, radius, slider.fillColor);
    painter.circle({node.position.x + (node.size.x * ratio), middle}, slider.handleRadius, slider.handleColor);
}

/// @brief Draws the caret of @p input, which has the focus, in @p node.
void paintCaret(Painter& painter, const text::FontAssets& fonts, const UiComputed& node, const UiText& shown,
                const UiTextInput& input) {
    const text::Font* font = fonts.get(shown.font);
    if (font == nullptr) {
        return;
    }
    constexpr float kCaretWidth = 2.0F;
    const auto [start, width] = contentOf(node);
    const std::size_t caret = std::min<std::size_t>(input.caret, shown.text.size());
    const float x = text::measureText(*font, std::string_view{shown.text}.substr(0, caret), shown.size).x;
    const float height = text::measureText(*font, "Ag", shown.size).y;
    painter.rectangle(start + glm::vec2{std::min(x, width), 0.0F}, {kCaretWidth, height}, 0.0F, input.caretColor);
}
}  // namespace

namespace {
/// @brief Fills the children of @p layout from the hierarchy.
/// @return The nodes without a parent node.
[[nodiscard]] std::vector<std::size_t> linkNodes(std::vector<UiLayoutNode>& layout,
                                                 const std::vector<const ecs::Children*>& hierarchy,
                                                 const std::vector<ecs::Entity>& parents,
                                                 const std::unordered_map<ecs::Entity, std::size_t>& indices) {
    std::vector<std::size_t> roots;
    for (std::size_t index = 0; index < layout.size(); ++index) {
        if (const ecs::Children* children = hierarchy.at(index)) {
            for (const ecs::Entity child : children->entities) {
                if (const auto found = indices.find(child); found != indices.end()) {
                    layout.at(index).children.push_back(found->second);
                }
            }
        }
        if (!indices.contains(parents.at(index))) {
            roots.push_back(index);
        }
    }
    return roots;
}

/// @return What each node of @p placed computes to, its order being its rank.
[[nodiscard]] std::unordered_map<ecs::Entity, UiComputed> toComputed(const std::vector<UiLaidOutNode>& placed) {
    std::unordered_map<ecs::Entity, UiComputed> results;
    results.reserve(placed.size());
    std::uint32_t order = 0;
    for (const UiLaidOutNode& node : placed) {
        results.emplace(node.entity, UiComputed{.position = node.position,
                                                .size = node.size,
                                                .contentSize = node.contentSize,
                                                .clip = node.clip,
                                                .padding = node.padding,
                                                .order = order});
        ++order;
    }
    return results;
}

/// @brief What a node draws.
struct DrawnNode {
    const UiComputed* computed;        ///< Where it is.
    const UiBackground* background;    ///< Its background, or null.
    const UiImage* image;              ///< Its image, or null.
    const UiText* shown;               ///< Its text, or null.
    const UiToggle* toggle;            ///< Its toggle, or null.
    const UiSlider* slider;            ///< Its slider, or null.
    const UiTextInput* input;          ///< Its input, or null.
    const UiInteraction* interaction;  ///< Its state, or null.
};

/// @brief Draws the text of @p node, or the placeholder of its input, and the caret.
void paintNodeText(Painter& painter, const text::FontAssets& fonts, const graphics::TextureAssets& textures,
                   const DrawnNode& node) {
    const bool placeholder = node.input != nullptr && node.shown->text.empty();
    Color color = node.shown->color;
    if (placeholder) {
        color.w *= 0.5F;
    }
    paintText(painter, fonts, textures, *node.computed, *node.shown,
              placeholder ? node.input->placeholder : node.shown->text, color);
    if (node.input != nullptr && node.interaction != nullptr && node.interaction->focused) {
        paintCaret(painter, fonts, *node.computed, *node.shown, *node.input);
    }
}

/// @brief Draws @p node, from its background up.
void paintNode(Painter& painter, const text::FontAssets& fonts, const graphics::TextureAssets& textures,
               const DrawnNode& node) {
    const UiComputed& computed = *node.computed;
    if (node.background != nullptr) {
        painter.rectangle(computed.position, computed.size, node.background->cornerRadius, node.background->color,
                          node.background->borderColor, node.background->borderWidth);
    }
    if (node.image != nullptr) {
        paintImage(painter, textures, computed, *node.image);
    }
    if (node.toggle != nullptr && node.toggle->checked) {
        const glm::vec2 inset{node.toggle->markInset};
        painter.rectangle(computed.position + inset, computed.size - (inset * 2.0F), 2.0F, node.toggle->markColor);
    }
    if (node.slider != nullptr) {
        paintSlider(painter, computed, *node.slider);
    }
    if (node.shown != nullptr) {
        paintNodeText(painter, fonts, textures, node);
    }
}
}  // namespace

void layoutUi(ecs::Res<graphics::ViewInfo> view, ecs::Res<text::FontAssets> fonts, ecs::Commands& commands,
              ecs::Query<const UiNode, ecs::Optional<const ecs::Parent>, ecs::Optional<const ecs::Children>,
                         ecs::Optional<const UiText>, ecs::Optional<const UiScroll>>
                  nodes,
              ecs::Query<UiComputed> computed) {
    std::vector<UiLayoutNode> layout;
    std::vector<const ecs::Children*> hierarchy;
    std::vector<ecs::Entity> parents;
    std::unordered_map<ecs::Entity, std::size_t> indices;
    for (auto [entity, node, parent, children, shown, scroll] : nodes) {
        indices.emplace(entity, layout.size());
        UiLayoutNode item{
            .entity = entity, .style = node, .scroll = scroll != nullptr ? scroll->offset : glm::vec2{0.0F}};
        if (shown != nullptr) {
            if (const text::Font* font = fonts->get(shown->font)) {
                item.measure = [font, shown](const float available) { return measure(*font, *shown, available); };
            }
        }
        layout.push_back(std::move(item));
        hierarchy.push_back(children);
        parents.push_back(parent != nullptr ? parent->entity : ecs::Entity{});
    }
    const std::vector<std::size_t> roots = linkNodes(layout, hierarchy, parents, indices);

    const std::vector<UiLaidOutNode> placed = layoutNodes(layout, roots, view->viewSize);
    std::unordered_map<ecs::Entity, UiComputed> results = toComputed(placed);
    for (auto [entity, result] : computed) {
        const auto found = results.find(entity);
        // A node that is not shown keeps a size of 0, so it can neither be drawn nor picked.
        *result = found != results.end() ? found->second : UiComputed{};
        if (found != results.end()) {
            results.erase(found);
        }
    }
    for (const auto& [entity, result] : results) {
        commands.entity(entity).add(result);
    }
}

void drawUi(ecs::ResAppend<graphics::OverlayQueue> overlay, ecs::Res<text::FontAssets> fonts,
            ecs::Res<graphics::TextureAssets> textures,
            ecs::Query<const UiComputed, ecs::Optional<const UiBackground>, ecs::Optional<const UiImage>,
                       ecs::Optional<const UiText>, ecs::Optional<const UiToggle>, ecs::Optional<const UiSlider>,
                       ecs::Optional<const UiTextInput>, ecs::Optional<const UiInteraction>>
                nodes) {
    std::vector<DrawnNode> sorted;
    for (auto [entity, computed, background, image, shown, toggle, slider, input, interaction] : nodes) {
        if (computed.size.x > 0.0F || computed.size.y > 0.0F) {
            sorted.push_back(DrawnNode{.computed = &computed,
                                       .background = background,
                                       .image = image,
                                       .shown = shown,
                                       .toggle = toggle,
                                       .slider = slider,
                                       .input = input,
                                       .interaction = interaction});
        }
    }
    std::ranges::sort(sorted, {}, [](const DrawnNode& node) { return node.computed->order; });

    std::vector<OverlayItem> items;
    for (const DrawnNode& node : sorted) {
        Painter painter{items, node.computed->clip};
        paintNode(painter, *fonts, *textures, node);
    }
    for (OverlayItem& item : items) {
        overlay->push(std::move(item));
    }
}
}  // namespace rtype::plugins::ui::detail
