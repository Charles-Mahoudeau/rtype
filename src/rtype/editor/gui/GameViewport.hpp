/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameViewport
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>

namespace rtype::editor::gui {

/// @brief Where the game is on the screen, written by the Game panel each frame and read by the input bridge to place
/// the mouse in the game and to know whether the game has the mouse or the keyboard.
struct GameViewport {
    glm::vec2 position{0.0F};  ///< Top-left corner of the picture of the game, in window units.
    glm::vec2 size{0.0F};      ///< Size of the picture, in window units; 0 while the panel is hidden.
    glm::uvec2 sceneSize{0};   ///< Size of the scene's texture, in pixels: the units the game sees its mouse in.
    bool hovered{false};       ///< Whether the mouse is over the picture, with no other panel in front.
    bool focused{false};       ///< Whether the Game panel has the keyboard focus.
};

}  // namespace rtype::editor::gui
