/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiWidgets
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <string>

#include "plugins/GlmReflection.hpp"       // IWYU pragma: keep
#include "plugins/GraphicsReflection.hpp"  // IWYU pragma: keep
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/String.hpp"

// The widgets are components put on a UiNode, next to what they draw: nothing here is a fixed look. A button is any
// node with a UiButton, whatever its children are.

namespace rtype::plugins::ui {

/// @brief Makes a node react to the pointer and to the focus; the UiPlugin keeps its state up to date.
struct UiInteraction {
    bool disabled = false;  ///< Ignores the pointer and the focus.
    bool focusable = true;  ///< Keyboard and gamepad navigation can reach it.
    bool hovered = false;   ///< The pointer is over it. Written by the plugin, as what follows.
    bool pressed = false;   ///< It is being held down.
    bool focused = false;   ///< It has the focus.
    bool clicked = false;   ///< It was activated in this frame.
};
RTYPE_ECS_COMPONENT(UiInteraction, "rtype.UiInteraction", disabled, focusable, hovered, pressed, focused, clicked);

/// @brief A button: colours the node's UiBackground or UiImage by state, and sends UiClicked with its action.
///
/// @details `action` is the game's own name for what the button does: a system reads UiClicked and looks at it, so one
/// reader serves a whole menu.
struct UiButton {
    ecs::String action;                                                 ///< Sent with UiClicked.
    contract::renderer::Color normalColor{1.0F};                        ///< At rest.
    contract::renderer::Color hoveredColor{0.85F, 0.85F, 0.85F, 1.0F};  ///< Under the pointer or focused.
    contract::renderer::Color pressedColor{0.7F, 0.7F, 0.7F, 1.0F};     ///< Held down.
    contract::renderer::Color disabledColor{0.5F, 0.5F, 0.5F, 0.5F};    ///< Disabled.
};
RTYPE_ECS_COMPONENT(UiButton, "rtype.UiButton", action, normalColor, hoveredColor, pressedColor, disabledColor);

/// @brief A check box or a switch: draws a mark in the node when it is on, and sends UiValueChanged with 0 or 1.
struct UiToggle {
    ecs::String action;                         ///< Sent with UiValueChanged.
    bool checked = false;                       ///< Whether it is on.
    contract::renderer::Color markColor{1.0F};  ///< The mark drawn when it is on.
    float markInset = 4.0F;                     ///< Space around the mark, in view pixels.
};
RTYPE_ECS_COMPONENT(UiToggle, "rtype.UiToggle", action, checked, markColor, markInset);

/// @brief A horizontal slider over the width of the node, driven by the pointer, the keyboard or a gamepad.
struct UiSlider {
    ecs::String action;         ///< Sent with UiValueChanged.
    float min = 0.0F;           ///< Its value at the left.
    float max = 1.0F;           ///< Its value at the right.
    float value = 0.0F;         ///< Its value.
    float step = 0.0F;          ///< What the value snaps to; 0: free, and 5% per key press.
    float trackHeight = 6.0F;   ///< In view pixels.
    float handleRadius = 9.0F;  ///< In view pixels.
    contract::renderer::Color trackColor{0.3F, 0.3F, 0.3F, 1.0F};  ///< The part right of the handle.
    contract::renderer::Color fillColor{0.2F, 0.6F, 1.0F, 1.0F};   ///< The part left of the handle.
    contract::renderer::Color handleColor{1.0F};                   ///< The handle.
};
RTYPE_ECS_COMPONENT(UiSlider, "rtype.UiSlider", action, min, max, value, step, trackHeight, handleRadius, trackColor,
                    fillColor, handleColor);

/// @brief Scrolls the content of a node whose overflow is UiOverflow::kScroll, with the wheel.
struct UiScroll {
    glm::vec2 offset{0.0F};  ///< How far the content moved, in view pixels, from (0, 0).
    float speed = 40.0F;     ///< View pixels per wheel step.
};
RTYPE_ECS_COMPONENT(UiScroll, "rtype.UiScroll", offset, speed);

/// @brief A single line of text to edit, on a node with a UiText, which shows it.
///
/// @details Clicking focuses it. Typing sends UiTextChanged and Enter sends UiTextSubmitted, with the text in the
/// node's UiText.
struct UiTextInput {
    ecs::String action;                          ///< Sent with UiTextChanged and UiTextSubmitted.
    std::string placeholder;                     ///< Shown, dimmed, while the text is empty.
    std::uint32_t maxLength = 0;                 ///< The most bytes it takes; 0: no limit.
    std::uint32_t caret = 0;                     ///< Where typing goes, in bytes from the start.
    contract::renderer::Color caretColor{1.0F};  ///< The caret, drawn while it has the focus.
};
RTYPE_ECS_COMPONENT(UiTextInput, "rtype.UiTextInput", action, placeholder, maxLength, caret, caretColor);

}  // namespace rtype::plugins::ui
