/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** View
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <variant>

namespace rtype::plugins::backend::graphics {

/// @brief Parallel projection, without perspective: what every 2D view is.
struct Orthographic {
    float zoom = 1.0F;  ///< View pixels (of the renderer's virtual resolution, or the window's) per world unit: 2 shows
                        ///< everything twice as big.
};

/// @brief How a view projects the world onto the screen.
/// @note Only Orthographic for now. A Perspective alternative will join the variant if 3D comes, without changing
/// View; until then, every backend (2D-only libraries included) can honour every projection.
using Projection = std::variant<Orthographic>;

/// @brief The point of view a frame is seen from, set with IRenderer::setView().
/// @details In an App, the backend plugin builds it from the Camera component with the highest priority and the
/// GlobalTransform of its entity.
struct View {
    glm::vec2 position{0.0F};                ///< World point at the centre of the screen.
    float rotation = 0.0F;                   ///< Around the view axis, in radians, clockwise on screen.
    Projection projection = Orthographic{};  ///< How the world is projected.
};

}  // namespace rtype::plugins::backend::graphics
