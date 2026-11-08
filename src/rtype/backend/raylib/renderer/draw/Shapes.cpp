/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Shapes
*/

#include "Shapes.hpp"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>

#include "backend/raylib/renderer/Convert.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"

namespace rtype::backend::raylib::renderer::draw {
namespace {

using contract::renderer::Shape;

constexpr int kCircleSegments = 36;                   ///< Segments of a full circle.
constexpr int kHalfSegments = kCircleSegments / 2;    ///< Segments of a half circle (capsule ends).
constexpr int kCornerSegments = kCircleSegments / 4;  ///< Segments of a rounded corner.
constexpr float kHalfTurn = 180.0F;                   ///< Degrees.
constexpr float kFullTurn = 360.0F;                   ///< Degrees.

/// @brief The colours of a shape, and which parts are drawn.
struct Paint {
    ::Color fill;
    ::Color outline;
    bool hasFill;
    bool hasOutline;
};

Paint paintOf(const Shape& shape) {
    return {.fill = toColor(shape.fillColor),
            .outline = toColor(shape.outlineColor),
            .hasFill = shape.fillColor.w > 0.0F,
            .hasOutline = shape.outlineThickness > 0.0F && shape.outlineColor.w > 0.0F};
}

void drawRect(const Shape& shape, const Paint& paint) {
    const glm::vec2 half = shape.size / 2.0F;
    if (shape.rotation == 0.0F) {
        const ::Rectangle rect{.x = shape.position.x - half.x,
                               .y = shape.position.y - half.y,
                               .width = shape.size.x,
                               .height = shape.size.y};
        const float shortest = std::max(std::min(shape.size.x, shape.size.y), 1.0F);
        const float roundness = std::clamp(2.0F * shape.cornerRadius / shortest, 0.0F, 1.0F);
        if (paint.hasFill) {
            DrawRectangleRounded(rect, roundness, kCornerSegments, paint.fill);
        }
        if (paint.hasOutline) {
            DrawRectangleRoundedLinesEx(rect, roundness, kCornerSegments, shape.outlineThickness, paint.outline);
        }
        return;
    }
    if (paint.hasFill) {
        const ::Rectangle rect{
            .x = shape.position.x, .y = shape.position.y, .width = shape.size.x, .height = shape.size.y};
        DrawRectanglePro(rect, toVector(half), shape.rotation * kDegreesPerRadian, paint.fill);
    }
    if (paint.hasOutline) {
        const std::array corners{glm::vec2{-half.x, -half.y}, glm::vec2{half.x, -half.y}, glm::vec2{half.x, half.y},
                                 glm::vec2{-half.x, half.y}};
        for (std::size_t index = 0; index < corners.size(); ++index) {
            const glm::vec2 from = shape.position + rotate(corners.at(index), shape.rotation);
            const glm::vec2 to = shape.position + rotate(corners.at((index + 1) % corners.size()), shape.rotation);
            DrawLineEx(toVector(from), toVector(to), shape.outlineThickness, paint.outline);
        }
    }
}

void drawCircle(const Shape& shape, const Paint& paint) {
    if (paint.hasFill) {
        DrawCircleV(toVector(shape.position), shape.radius, paint.fill);
    }
    if (paint.hasOutline) {
        DrawRing(toVector(shape.position), std::max(shape.radius - shape.outlineThickness, 0.0F), shape.radius, 0.0F,
                 kFullTurn, kCircleSegments, paint.outline);
    }
}

void drawCapsule(const Shape& shape, const Paint& paint) {
    const float degrees = shape.rotation * kDegreesPerRadian;
    const glm::vec2 axis = rotate({0.0F, shape.halfHeight}, shape.rotation);
    const ::Vector2 top = toVector(shape.position - axis);
    const ::Vector2 bottom = toVector(shape.position + axis);
    if (paint.hasFill) {
        const ::Rectangle body{.x = shape.position.x,
                               .y = shape.position.y,
                               .width = shape.radius * 2.0F,
                               .height = shape.halfHeight * 2.0F};
        DrawRectanglePro(body, {.x = shape.radius, .y = shape.halfHeight}, degrees, paint.fill);
        DrawCircleSector(top, shape.radius, degrees + kHalfTurn, degrees + kFullTurn, kHalfSegments, paint.fill);
        DrawCircleSector(bottom, shape.radius, degrees, degrees + kHalfTurn, kHalfSegments, paint.fill);
    }
    if (paint.hasOutline) {
        const float inner = std::max(shape.radius - shape.outlineThickness, 0.0F);
        const glm::vec2 side = rotate({shape.radius - (shape.outlineThickness / 2.0F), 0.0F}, shape.rotation);
        DrawLineEx(toVector(shape.position - axis + side), toVector(shape.position + axis + side),
                   shape.outlineThickness, paint.outline);
        DrawLineEx(toVector(shape.position - axis - side), toVector(shape.position + axis - side),
                   shape.outlineThickness, paint.outline);
        DrawRing(top, inner, shape.radius, degrees + kHalfTurn, degrees + kFullTurn, kHalfSegments, paint.outline);
        DrawRing(bottom, inner, shape.radius, degrees, degrees + kHalfTurn, kHalfSegments, paint.outline);
    }
}

void drawLine(const Shape& shape, const Paint& paint) {
    const ::Color color = paint.hasFill ? paint.fill : paint.outline;
    DrawLineEx(toVector(shape.position), toVector(shape.end), shape.radius * 2.0F, color);
    DrawCircleV(toVector(shape.position), shape.radius, color);
    DrawCircleV(toVector(shape.end), shape.radius, color);
}

}  // namespace

void drawShape(const Shape& shape) {
    const Paint paint = paintOf(shape);
    switch (shape.kind) {
        case contract::renderer::ShapeKind::kRect:
            drawRect(shape, paint);
            break;
        case contract::renderer::ShapeKind::kCircle:
            drawCircle(shape, paint);
            break;
        case contract::renderer::ShapeKind::kCapsule:
            drawCapsule(shape, paint);
            break;
        case contract::renderer::ShapeKind::kLine:
            drawLine(shape, paint);
            break;
    }
}

}  // namespace rtype::backend::raylib::renderer::draw
