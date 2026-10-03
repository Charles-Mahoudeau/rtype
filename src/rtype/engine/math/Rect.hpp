/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Rect
*/

#pragma once

#include <glm/vec2.hpp>

namespace rtype::engine::math {
class Rect {
  public:
    Rect() = default;
    Rect(glm::vec2 position, glm::vec2 size) : _x(position.x), _y(position.y), _width(size.x), _height(size.y) {}

    [[nodiscard]] float getX() const { return _x; }
    [[nodiscard]] float getY() const { return _y; }
    [[nodiscard]] float getWidth() const { return _width; }
    [[nodiscard]] float getHeight() const { return _height; }

  private:
    float _x{};
    float _y{};
    float _width{};
    float _height{};
};
}  // namespace rtype::engine::math
