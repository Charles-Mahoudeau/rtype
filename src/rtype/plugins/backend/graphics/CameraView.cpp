/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CameraView
*/

#include "plugins/backend/graphics/CameraView.hpp"

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/matrix.hpp>

#include "plugins/backend/graphics/Camera.hpp"

namespace rtype::plugins::backend::graphics {

namespace {

/// @return @p viewSize, or the framebuffer's size when it is 0x0.
glm::vec2 resolveViewSize(glm::vec2 viewSize, glm::uvec2 framebufferSize) {
    return viewSize.x > 0.0F && viewSize.y > 0.0F ? viewSize : glm::vec2{framebufferSize};
}

/// @return The largest rectangle of @p viewSize's aspect ratio in @p framebufferSize, centred.
Viewport letterbox(glm::vec2 viewSize, glm::uvec2 framebufferSize) {
    const glm::vec2 framebuffer{framebufferSize};
    if (viewSize.x <= 0.0F || viewSize.y <= 0.0F) {
        return {};
    }
    const float scale = std::min(framebuffer.x / viewSize.x, framebuffer.y / viewSize.y);
    const glm::vec2 size = viewSize * scale;
    return Viewport{.position = (framebuffer - size) * 0.5F, .size = size};
}

}  // namespace

CameraView::CameraView(const Camera& camera, glm::vec2 viewSize, glm::uvec2 framebufferSize)
    : _viewport{letterbox(resolveViewSize(viewSize, framebufferSize), framebufferSize)} {
    const glm::vec2 size = resolveViewSize(viewSize, framebufferSize);
    const float zoom = std::get<Orthographic>(camera.projection).zoom;
    const glm::vec2 half = size / (2.0F * zoom);
    const glm::mat4 projection = glm::orthoRH_ZO(-half.x, half.x, -half.y, half.y, 0.0F, 1.0F);
    const glm::mat4 view = glm::translate(glm::rotate(glm::mat4{1.0F}, -camera.rotation, glm::vec3{0.0F, 0.0F, 1.0F}),
                                          glm::vec3{-camera.position.x, -camera.position.y, 0.0F});
    _viewProjection = projection * view;
    _inverse = glm::inverse(_viewProjection);
}

Camera CameraView::makeDefaultCamera(glm::vec2 viewSize, glm::uvec2 framebufferSize) {
    const glm::vec2 centre = resolveViewSize(viewSize, framebufferSize) * 0.5F;
    return Camera{.position = {centre.x, centre.y, 0.0F}};
}

glm::vec2 CameraView::screenToWorld(glm::vec2 screen) const {
    const glm::vec2 clip = ((screen - _viewport.position) / _viewport.size * 2.0F) - 1.0F;
    const glm::vec4 world = _inverse * glm::vec4{clip.x, clip.y, 0.0F, 1.0F};
    return {world.x, world.y};
}

glm::vec2 CameraView::worldToScreen(glm::vec2 world) const {
    const glm::vec4 clip = _viewProjection * glm::vec4{world.x, world.y, 0.0F, 1.0F};
    return _viewport.position + ((glm::vec2{clip.x, clip.y} + 1.0F) * 0.5F * _viewport.size);
}

}  // namespace rtype::plugins::backend::graphics
