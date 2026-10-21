/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapeInstanceTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shape.hpp"
#include "render/vulkan/rendering/ShapeInstance.hpp"

namespace {
using rtype::plugins::backend::graphics::Shape;
using rtype::plugins::backend::graphics::ShapeKind;
using rtype::render::vulkan::rendering::ShapeInstance;
using rtype::render::vulkan::rendering::ShapeInstanceKind;

constexpr float kTolerance = 1e-5F;

/// @return Where the shaders' rotation takes a local point: the same matrix as shape.vert.
glm::vec2 rotate(glm::vec2 local, float rotation) {
    const float c = std::cos(rotation);
    const float s = std::sin(rotation);
    return {(c * local.x) - (s * local.y), (s * local.x) + (c * local.y)};
}
}  // namespace

TEST(ShapeInstance, ARectKeepsItsHalfSizeRotationAndCornerRadius) {
    const ShapeInstance instance = ShapeInstance::fromShape(Shape{.kind = ShapeKind::kRect,
                                                                  .position = {10.0F, 20.0F},
                                                                  .size = {64.0F, 32.0F},
                                                                  .cornerRadius = 4.0F,
                                                                  .rotation = 0.5F});
    EXPECT_EQ(instance.kind, ShapeInstanceKind::kRect);
    EXPECT_EQ(instance.center, (glm::vec2{10.0F, 20.0F}));
    EXPECT_EQ(instance.halfSize, (glm::vec2{32.0F, 16.0F}));
    EXPECT_FLOAT_EQ(instance.radius, 4.0F);
    EXPECT_FLOAT_EQ(instance.rotation, 0.5F);
}

TEST(ShapeInstance, ARectCornerIsNeverRounderThanHalfItsShorterSide) {
    const ShapeInstance instance =
        ShapeInstance::fromShape(Shape{.kind = ShapeKind::kRect, .size = {64.0F, 10.0F}, .cornerRadius = 50.0F});
    EXPECT_FLOAT_EQ(instance.radius, 5.0F);
}

TEST(ShapeInstance, ACircleIsAsWideAsItsRadiusAndNeverTurns) {
    const ShapeInstance instance =
        ShapeInstance::fromShape(Shape{.kind = ShapeKind::kCircle, .radius = 8.0F, .rotation = 1.0F});
    EXPECT_EQ(instance.kind, ShapeInstanceKind::kCircle);
    EXPECT_EQ(instance.halfSize, (glm::vec2{8.0F, 8.0F}));
    EXPECT_FLOAT_EQ(instance.radius, 8.0F);
    EXPECT_FLOAT_EQ(instance.rotation, 0.0F);
}

TEST(ShapeInstance, ACapsuleCoversItsStraightPartAndCaps) {
    const ShapeInstance instance =
        ShapeInstance::fromShape(Shape{.kind = ShapeKind::kCapsule, .radius = 8.0F, .halfHeight = 16.0F});
    EXPECT_EQ(instance.kind, ShapeInstanceKind::kCapsule);
    EXPECT_EQ(instance.halfSize, (glm::vec2{8.0F, 24.0F}));  // The straight part plus a cap above and below.
    EXPECT_FLOAT_EQ(instance.radius, 8.0F);
}

TEST(ShapeInstance, ALineIsACapsuleCentredOnItsSegment) {
    const ShapeInstance instance = ShapeInstance::fromShape(
        Shape{.kind = ShapeKind::kLine, .position = {0.0F, 0.0F}, .end = {30.0F, 40.0F}, .radius = 2.0F});
    EXPECT_EQ(instance.kind, ShapeInstanceKind::kCapsule);
    EXPECT_EQ(instance.center, (glm::vec2{15.0F, 20.0F}));
    EXPECT_FLOAT_EQ(instance.halfSize.x, 2.0F);
    EXPECT_FLOAT_EQ(instance.halfSize.y, 27.0F);  // Half the 50-long segment, plus the cap.
}

TEST(ShapeInstance, ALineTurnsItsCapsuleOntoTheSegment) {
    const glm::vec2 start{10.0F, 10.0F};
    const glm::vec2 end{-20.0F, 50.0F};
    const ShapeInstance instance =
        ShapeInstance::fromShape(Shape{.kind = ShapeKind::kLine, .position = start, .end = end, .radius = 1.0F});
    // The capsule's axis ends, local (0, +-halfLength), must land on the segment's ends.
    const float halfLength = instance.halfSize.y - instance.radius;
    const glm::vec2 tip = instance.center + rotate({0.0F, halfLength}, instance.rotation);
    const glm::vec2 tail = instance.center + rotate({0.0F, -halfLength}, instance.rotation);
    EXPECT_NEAR(tip.x, end.x, kTolerance * 100.0F);
    EXPECT_NEAR(tip.y, end.y, kTolerance * 100.0F);
    EXPECT_NEAR(tail.x, start.x, kTolerance * 100.0F);
    EXPECT_NEAR(tail.y, start.y, kTolerance * 100.0F);
}

TEST(ShapeInstance, ADegenerateLineIsADot) {
    const ShapeInstance instance = ShapeInstance::fromShape(
        Shape{.kind = ShapeKind::kLine, .position = {5.0F, 5.0F}, .end = {5.0F, 5.0F}, .radius = 3.0F});
    EXPECT_EQ(instance.center, (glm::vec2{5.0F, 5.0F}));
    EXPECT_EQ(instance.halfSize, (glm::vec2{3.0F, 3.0F}));
    EXPECT_FLOAT_EQ(instance.rotation, 0.0F);
}

TEST(ShapeInstance, KeepsItsColoursAndOutline) {
    const ShapeInstance instance = ShapeInstance::fromShape(Shape{.kind = ShapeKind::kCircle,
                                                                  .radius = 4.0F,
                                                                  .fillColor = {0.0F, 0.0F, 0.0F, 0.0F},
                                                                  .outlineColor = {0.0F, 1.0F, 0.0F, 1.0F},
                                                                  .outlineThickness = 1.5F});
    EXPECT_EQ(instance.fillColor, (glm::vec4{0.0F, 0.0F, 0.0F, 0.0F}));
    EXPECT_EQ(instance.outlineColor, (glm::vec4{0.0F, 1.0F, 0.0F, 1.0F}));
    EXPECT_FLOAT_EQ(instance.outlineThickness, 1.5F);
}

TEST(ShapeInstance, NegativeSizesAreClampedToZero) {
    const ShapeInstance instance = ShapeInstance::fromShape(
        Shape{.kind = ShapeKind::kCapsule, .radius = -4.0F, .halfHeight = -2.0F, .outlineThickness = -1.0F});
    EXPECT_EQ(instance.halfSize, (glm::vec2{0.0F, 0.0F}));
    EXPECT_FLOAT_EQ(instance.radius, 0.0F);
    EXPECT_FLOAT_EQ(instance.outlineThickness, 0.0F);
}

TEST(ShapeInstance, DescribesEightPerInstanceAttributes) {
    const auto bindings = ShapeInstance::getBindings(2);
    EXPECT_EQ(bindings.at(0).binding, 2U);
    EXPECT_EQ(bindings.at(0).stride, sizeof(ShapeInstance));
    EXPECT_EQ(bindings.at(0).inputRate, vk::VertexInputRate::eInstance);
    const auto attributes = ShapeInstance::getAttributes(2);
    ASSERT_EQ(attributes.size(), 8U);
    for (std::size_t location = 0; location < attributes.size(); ++location) {
        EXPECT_EQ(attributes.at(location).location, location);
        EXPECT_EQ(attributes.at(location).binding, 2U);
    }
    EXPECT_EQ(attributes.at(5).format, vk::Format::eR32Uint);  // The kind is an integer.
    EXPECT_EQ(attributes.at(5).offset, offsetof(ShapeInstance, kind));
}
