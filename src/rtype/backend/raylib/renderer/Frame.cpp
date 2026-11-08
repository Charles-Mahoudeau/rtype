/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Frame
*/

#include "Frame.hpp"

#include <raylib.h>

#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <variant>

#include "Convert.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Overlay.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/contract/renderer/view/CameraView.hpp"
#include "rtype/contract/renderer/view/View.hpp"

namespace rtype::backend::raylib::renderer {
namespace {

/// @return The size of the window's framebuffer, in pixels.
glm::uvec2 framebufferSize() {
    return {static_cast<unsigned>(GetRenderWidth()), static_cast<unsigned>(GetRenderHeight())};
}

}  // namespace

Frame::Frame(std::optional<glm::uvec2> virtualResolution) : _virtualResolution(virtualResolution) {}

glm::vec2 Frame::getViewSize() const {
    return _virtualResolution.has_value() ? glm::vec2(_virtualResolution.value()) : glm::vec2(0.0F);
}

void Frame::begin(const contract::renderer::Color& clearColor) {
    BeginDrawing();
    ClearBackground(BLACK);
    place(contract::renderer::CameraView::makeDefaultView(getViewSize(), framebufferSize()));
    DrawRectangleRec(_viewport, toColor(clearColor));
    BeginBlendMode(BLEND_ALPHA);
    _blend = BLEND_ALPHA;
    beginCamera();
    _drawing = true;
}

void Frame::end() {
    if (!_drawing) {
        return;
    }
    endCamera();
    EndBlendMode();
    EndDrawing();
    _drawing = false;
}

void Frame::setView(const contract::renderer::View& view) {
    if (_drawing) {
        endCamera();
    }
    place(view);
    if (_drawing) {
        beginCamera();
    }
}

void Frame::useBlend(contract::renderer::BlendMode blend) {
    const int wanted = toBlend(blend);
    if (wanted != _blend) {
        EndBlendMode();
        BeginBlendMode(wanted);
        _blend = wanted;
    }
}

void Frame::forEachOverlayItem(std::span<const contract::renderer::OverlayItem> items,
                               const std::function<void(const contract::renderer::OverlayItem&)>& draw) {
    if (!_drawing || items.empty()) {
        return;
    }
    const ::Camera2D world = _camera;
    const ::Rectangle viewport = _viewport;
    endCamera();
    _camera = {
        .offset = {.x = viewport.x, .y = viewport.y}, .target = {.x = 0.0F, .y = 0.0F}, .rotation = 0.0F, .zoom = _fit};
    for (const contract::renderer::OverlayItem& item : items) {
        if (item.clip.has_value()) {
            const contract::renderer::math::Rect& clip = item.clip.value();
            _viewport = {.x = viewport.x + (clip.getX() * _fit),
                         .y = viewport.y + (clip.getY() * _fit),
                         .width = clip.getWidth() * _fit,
                         .height = clip.getHeight() * _fit};
        }
        beginCamera();
        draw(item);
        endCamera();
        _viewport = viewport;
    }
    _camera = world;
    beginCamera();
}

void Frame::inScreenSpace(const std::function<void()>& draw) {
    if (!_drawing) {
        return;
    }
    endCamera();
    draw();
    beginCamera();
}

void Frame::place(const contract::renderer::View& view) {
    const glm::uvec2 framebuffer = framebufferSize();
    const contract::renderer::CameraView shown{view, getViewSize(), framebuffer};
    const contract::renderer::Viewport& area = shown.getViewport();
    _viewport = {.x = area.position.x, .y = area.position.y, .width = area.size.x, .height = area.size.y};
    const glm::vec2 viewSize = _virtualResolution.has_value() ? getViewSize() : glm::vec2(framebuffer);
    _fit = viewSize.x > 0.0F ? area.size.x / viewSize.x : 1.0F;
    const auto* orthographic = std::get_if<contract::renderer::Orthographic>(&view.projection);
    const float zoom = orthographic != nullptr ? orthographic->zoom : 1.0F;
    _camera = {.offset = {.x = area.position.x + (area.size.x / 2.0F), .y = area.position.y + (area.size.y / 2.0F)},
               .target = toVector(view.position),
               .rotation = -view.rotation * kDegreesPerRadian,
               .zoom = _fit * zoom};
}

void Frame::beginCamera() const {
    BeginScissorMode(static_cast<int>(_viewport.x), static_cast<int>(_viewport.y), static_cast<int>(_viewport.width),
                     static_cast<int>(_viewport.height));
    BeginMode2D(_camera);
}

void Frame::endCamera() {
    EndMode2D();
    EndScissorMode();
}

}  // namespace rtype::backend::raylib::renderer
