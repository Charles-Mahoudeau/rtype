/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextRenderer
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <type_traits>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/text/TextLayout.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

/// @brief A text::TextAlign field is reflected as kU8, as a BlendMode is (see SpriteRenderer.hpp).
template <>
struct rtype::ecs::FieldTypeOf<rtype::plugins::text::TextAlign>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::plugins::text::TextAlign) == 1, "TextAlign must match FieldType::kU8");

namespace rtype::plugins::text {
/// @brief Draws a text on an entity: the component the TextPlugin lays out (layoutText()) and draws with the sprites.
///
/// @details Only what to draw: where (position, scale, rotation) comes from the entity's GlobalTransform, the anchor
/// saying which point of the text sits there. The font is referred to by its AssetId, which FontAssets::load() returns
/// at once; the text is drawn once its font and its atlas are loaded, sorted with the sprites by layer. Every field but
/// `text` is reflected (Luau, network): the ECS reflects no strings yet.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {400.0F, 300.0F}})
///     .add(rtype::plugins::text::TextRenderer{.text = "Player 1 wins", .font = orbitron, .size = 48.0F,
///                                             .anchor = {0.5F, 0.5F}});
/// @endcode
struct TextRenderer {
    std::string text;                      ///< UTF-8; `\n` starts a new line.
    core::asset::AssetId font;             ///< From FontAssets::load().
    float size = 32.0F;                    ///< The font size: world units per em, before the entity's scale.
    backend::graphics::Color color{1.0F};  ///< Fills the glyphs: colour and transparency.
    TextAlign align = TextAlign::kLeft;    ///< How the lines line up, for a text of several lines.
    glm::vec2 anchor{0.0F};  ///< The point of the text on the entity's position: (0, 0) its top-left corner, (0.5, 0.5)
                             ///< its centre, (1, 1) its bottom-right corner. It is also the pivot of the rotation.
    std::int32_t layer{0};   ///< Draw order among the sprites: higher layers are drawn on top.
    backend::graphics::BlendMode blend = backend::graphics::BlendMode::kAlpha;  ///< kAlpha or kAdditive.
};
RTYPE_ECS_COMPONENT(TextRenderer, "rtype.TextRenderer", font, size, color, align, anchor, layer);
}  // namespace rtype::plugins::text
