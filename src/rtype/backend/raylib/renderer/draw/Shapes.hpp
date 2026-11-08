/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Shapes
*/

#pragma once

#include "rtype/contract/renderer/drawing/Shape.hpp"

namespace rtype::backend::raylib::renderer::draw {

/// @brief Draws @p shape with raylib's shape functions: its fill, then its outline, drawn inside the shape.
/// @details A rotated rectangle loses its rounded corners: raylib only rounds rectangles that are not turned.
void drawShape(const contract::renderer::Shape& shape);

}  // namespace rtype::backend::raylib::renderer::draw
