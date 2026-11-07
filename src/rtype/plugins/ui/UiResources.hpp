/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiResources
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::ui {

/// @brief The pointer as the UI sees it, as the ECS resource "rtype.UiPointer".
struct UiPointer {
    glm::vec2 position{0.0F};  ///< In view pixels.
    bool overUi = false;       ///< It is over something of the UI: gameplay should ignore clicks. Nothing blocks them.
    ecs::Entity hovered;       ///< The UiInteraction node under it, or null.
    ecs::Entity pressed;       ///< The UiInteraction node it holds down, or null.
    ecs::Entity clicked;       ///< The UiInteraction node it activated in this frame, or null.
};
RTYPE_ECS_RESOURCE(UiPointer, "rtype.UiPointer", position, overUi, hovered, pressed, clicked);

/// @brief What has the keyboard and the gamepad, as the ECS resource "rtype.UiFocus".
struct UiFocus {
    ecs::Entity focused;     ///< The focused UiInteraction node, or null.
    bool typing = false;     ///< It is a UiTextInput: gameplay should ignore the keyboard.
    bool activated = false;  ///< The keyboard or the gamepad activated it in this frame.
    float adjust = 0.0F;     ///< Left or right was pressed on a slider in this frame: -1 or 1.
};
RTYPE_ECS_RESOURCE(UiFocus, "rtype.UiFocus", focused, typing, activated, adjust);

}  // namespace rtype::plugins::ui
