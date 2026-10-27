/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VisibleArea
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief The part of the world the frame shows, as the ECS resource "rtype.VisibleArea": systems that draw many
/// things (the tiles of a tilemap, particles) skip what is outside it.
///
/// @details The BackendPlugin's Render system "rtype.backend.camera" updates it each frame from the camera shown, the
/// renderer's view size and the framebuffer: it is the box around what the viewport shows, so a rotated view gives a
/// bigger box. Systems that read it run after that system. It is empty before the first frame, on a server and while
/// the window is minimized.
struct VisibleArea {
    glm::vec2 min{0.0F};  ///< Top-left corner, in world units.
    glm::vec2 max{0.0F};  ///< Bottom-right corner, in world units.

    /// @return Whether it shows nothing.
    [[nodiscard]] bool isEmpty() const noexcept { return max.x <= min.x || max.y <= min.y; }

    /// @return Whether the box from @p boxMin to @p boxMax, in world units, is at least partly shown.
    [[nodiscard]] bool overlaps(glm::vec2 boxMin, glm::vec2 boxMax) const noexcept {
        return boxMin.x < max.x && boxMax.x > min.x && boxMin.y < max.y && boxMax.y > min.y;
    }
};
RTYPE_ECS_RESOURCE(VisibleArea, "rtype.VisibleArea");

}  // namespace rtype::plugins::backend::graphics
