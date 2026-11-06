/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformMathTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <glm/ext/vector_float2.hpp>
#include <numbers>

#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"

using rtype::plugins::transform::combine;
using rtype::plugins::transform::GlobalTransform;
using rtype::plugins::transform::toLocal;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;
constexpr float kTolerance = 1e-3F;

void expectNear(glm::vec2 actual, glm::vec2 expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}
}  // namespace

TEST(Combine, IsTheLocalTransformUnderAnIdentityParent) {
    const GlobalTransform global =
        combine(GlobalTransform{}, Transform{.position = {3.0F, 4.0F}, .rotation = 0.5F, .scale = {2.0F, 2.0F}});
    expectNear(global.position, {3.0F, 4.0F});
    EXPECT_NEAR(global.rotation, 0.5F, kTolerance);
    expectNear(global.scale, {2.0F, 2.0F});
}

TEST(Combine, AddsThePositions) {
    expectNear(combine(GlobalTransform{.position = {400.0F, 200.0F}}, Transform{.position = {-30.0F, 10.0F}}).position,
               {370.0F, 210.0F});
}

TEST(Combine, TurnsThePositionClockwiseOnScreen) {
    const GlobalTransform parent{.rotation = kQuarterTurn};
    expectNear(combine(parent, Transform{.position = {10.0F, 0.0F}}).position, {0.0F, 10.0F});
}

TEST(Combine, ScalesThePositionBeforeTurningIt) {
    const GlobalTransform parent{.rotation = kQuarterTurn, .scale = {2.0F, 3.0F}};
    expectNear(combine(parent, Transform{.position = {10.0F, 0.0F}}).position, {0.0F, 20.0F});
}

TEST(Combine, MultipliesTheScalesAndAddsTheRotations) {
    const GlobalTransform global = combine(GlobalTransform{.rotation = 0.25F, .scale = {2.0F, 3.0F}},
                                           Transform{.rotation = 0.5F, .scale = {4.0F, 5.0F}});
    EXPECT_NEAR(global.rotation, 0.75F, kTolerance);
    expectNear(global.scale, {8.0F, 15.0F});
}

TEST(Combine, MirrorsThePositionAndReversesTheRotation) {
    const GlobalTransform global =
        combine(GlobalTransform{.scale = {-1.0F, 1.0F}}, Transform{.position = {10.0F, 5.0F}, .rotation = 0.5F});
    expectNear(global.position, {-10.0F, 5.0F});
    EXPECT_NEAR(global.rotation, -0.5F, kTolerance);
    expectNear(global.scale, {-1.0F, 1.0F});
}

TEST(Combine, KeepsTheRotationUnderAMirrorOnBothAxes) {
    EXPECT_NEAR(combine(GlobalTransform{.scale = {-1.0F, -1.0F}}, Transform{.rotation = 0.5F}).rotation, 0.5F,
                kTolerance);
}

TEST(Combine, ChainsThroughSeveralLevels) {
    const GlobalTransform root{.position = {400.0F, 200.0F}};
    const GlobalTransform turret = combine(root, Transform{.position = {30.0F, 10.0F}});
    expectNear(combine(turret, Transform{.position = {0.0F, -8.0F}}).position, {430.0F, 202.0F});
}

TEST(ToLocal, UndoesCombine) {
    constexpr std::array kParents{
        GlobalTransform{},
        GlobalTransform{.position = {100.0F, -50.0F}},
        GlobalTransform{.rotation = 1.2F},
        GlobalTransform{.scale = {2.0F, 2.0F}},
        GlobalTransform{.scale = {-1.0F, 1.0F}},
        GlobalTransform{.scale = {1.0F, -1.0F}},
        GlobalTransform{.scale = {-2.0F, -2.0F}},
        GlobalTransform{.position = {10.0F, 20.0F}, .rotation = -0.7F, .scale = {3.0F, 3.0F}},
        GlobalTransform{.position = {-5.0F, 8.0F}, .rotation = 2.1F, .scale = {-1.5F, 1.5F}},
        GlobalTransform{.position = {7.0F, 7.0F}, .rotation = 0.4F, .scale = {2.0F, 0.5F}},
    };
    const Transform local{.position = {12.0F, -7.0F}, .rotation = 0.6F, .scale = {1.5F, 0.75F}};
    for (const GlobalTransform& parent : kParents) {
        const Transform back = toLocal(parent, combine(parent, local));
        expectNear(back.position, local.position);
        EXPECT_NEAR(back.rotation, local.rotation, kTolerance);
        expectNear(back.scale, local.scale);
    }
}

TEST(ToLocal, GivesZeroOnAnAxisTheParentFlattens) {
    const Transform local = toLocal(GlobalTransform{.scale = {0.0F, 2.0F}},
                                    GlobalTransform{.position = {10.0F, 10.0F}, .scale = {1.0F, 1.0F}});
    EXPECT_FALSE(std::isnan(local.position.x));
    expectNear(local.position, {0.0F, 5.0F});
    expectNear(local.scale, {0.0F, 0.5F});
}
