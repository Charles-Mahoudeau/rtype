/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CameraViewTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <numbers>

#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/View.hpp"

namespace {
using rtype::plugins::backend::graphics::CameraView;
using rtype::plugins::backend::graphics::View;

constexpr float kTolerance = 1e-3F;

/// @brief Checks two points are equal within kTolerance.
void expectNear(glm::vec2 actual, glm::vec2 expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}
}  // namespace

TEST(CameraView, DefaultViewShowsWindowPixelsFromTheTopLeft) {
    const glm::uvec2 framebuffer{800, 600};
    const CameraView view{CameraView::makeDefaultView({0.0F, 0.0F}, framebuffer), {0.0F, 0.0F}, framebuffer};
    expectNear(view.worldToScreen({0.0F, 0.0F}), {0.0F, 0.0F});
    expectNear(view.worldToScreen({800.0F, 600.0F}), {800.0F, 600.0F});
    expectNear(view.getViewport().size, {800.0F, 600.0F});
}

TEST(CameraView, MapsTheTopOfTheWorldToTheTopOfClipSpace) {
    const CameraView view{View{}, {0.0F, 0.0F}, {800, 600}};
    const glm::vec4 top = view.getViewProjection() * glm::vec4{0.0F, -300.0F, 0.0F, 1.0F};
    EXPECT_NEAR(top.y, -1.0F, kTolerance);
}

TEST(CameraView, CentresTheCameraPosition) {
    const CameraView view{View{.position = {100.0F, 50.0F}}, {0.0F, 0.0F}, {800, 600}};
    expectNear(view.worldToScreen({100.0F, 50.0F}), {400.0F, 300.0F});
}

TEST(CameraView, ZoomShowsTheWorldBigger) {
    const CameraView view{
        View{.projection = rtype::plugins::backend::graphics::Orthographic{.zoom = 2.0F}}, {0.0F, 0.0F}, {800, 600}};
    expectNear(view.worldToScreen({10.0F, 0.0F}), {420.0F, 300.0F});
}

TEST(CameraView, LetterboxesAWiderViewInATallWindow) {
    const CameraView view{View{}, {1920.0F, 1080.0F}, {800, 800}};
    expectNear(view.getViewport().position, {0.0F, 175.0F});
    expectNear(view.getViewport().size, {800.0F, 450.0F});
}

TEST(CameraView, PillarboxesANarrowerView) {
    const CameraView view{View{}, {1000.0F, 1000.0F}, {1600, 800}};
    expectNear(view.getViewport().position, {400.0F, 0.0F});
    expectNear(view.getViewport().size, {800.0F, 800.0F});
}

TEST(CameraView, ShowsTheSameWorldWhateverTheWindowSize) {
    const View view{.position = {960.0F, 540.0F}};
    const CameraView small{view, {1920.0F, 1080.0F}, {640, 360}};
    const CameraView big{view, {1920.0F, 1080.0F}, {1920, 1080}};
    expectNear(small.screenToWorld({640.0F, 360.0F}), {1920.0F, 1080.0F});
    expectNear(big.screenToWorld({1920.0F, 1080.0F}), {1920.0F, 1080.0F});
}

TEST(CameraView, RotatesClockwiseOnScreen) {
    // A camera turned a quarter clockwise sees the world turned a quarter counterclockwise: a point to the right of
    // the camera appears above it (Y down).
    const CameraView view{View{.rotation = std::numbers::pi_v<float> / 2.0F}, {0.0F, 0.0F}, {800, 600}};
    expectNear(view.worldToScreen({100.0F, 0.0F}), {400.0F, 200.0F});
}

TEST(CameraView, ScreenToWorldUndoesWorldToScreen) {
    const CameraView view{View{.position = {12.0F, -7.0F},
                               .rotation = 0.4F,
                               .projection = rtype::plugins::backend::graphics::Orthographic{.zoom = 1.5F}},
                          {1920.0F, 1080.0F},
                          {1280, 900}};
    expectNear(view.screenToWorld(view.worldToScreen({33.0F, 44.0F})), {33.0F, 44.0F});
}
