/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CameraView
*/

#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "plugins/backend/graphics/Camera.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief Where a frame is drawn in the window: a rectangle in framebuffer pixels, from the top-left corner.
struct Viewport {
    glm::vec2 position{0.0F};  ///< Top-left corner.
    glm::vec2 size{0.0F};      ///< Width and height.
};

/// @brief What a Camera shows in a window, and the conversions between window and world.
///
/// @details The view size is the resolution the game is designed for (e.g. 1920x1080): the camera shows that many
/// world units divided by its zoom, centred on its position, whatever the window size. The viewport is the largest
/// rectangle of the view's aspect ratio that fits in the framebuffer, centred: the rest is left as bars (letterboxing),
/// so nothing is stretched when the window is resized. Without a view size, the view is the framebuffer itself: one
/// world unit per pixel at zoom 1, no bars.
/// @code
/// const CameraView view{camera, {1920.0F, 1080.0F}, platform.getFramebufferSize()};
/// const glm::vec2 aim = view.screenToWorld(mousePosition);
/// @endcode
class CameraView {
  public:
    /// @param camera What to show; its projection must be Orthographic.
    /// @param viewSize The resolution the game is designed for; 0x0 for the framebuffer's size.
    /// @param framebufferSize The window's framebuffer, in pixels; a 0x0 one (minimized) gives an empty viewport.
    CameraView(const Camera& camera, glm::vec2 viewSize, glm::uvec2 framebufferSize);
    ~CameraView() = default;
    CameraView(const CameraView& other) = default;
    CameraView& operator=(const CameraView& other) = default;
    CameraView(CameraView&& other) noexcept = default;
    CameraView& operator=(CameraView&& other) noexcept = default;

    /// @return The camera IRenderer uses until setCamera() is called: the view's top-left corner at the world's
    /// origin, one world unit per view pixel.
    [[nodiscard]] static Camera makeDefaultCamera(glm::vec2 viewSize, glm::uvec2 framebufferSize);

    /// @return World to clip space: X right, Y down, depth 0 to 1 (Vulkan's conventions).
    [[nodiscard]] const glm::mat4& getViewProjection() const noexcept { return _viewProjection; }
    /// @return Where the frame goes in the framebuffer.
    [[nodiscard]] const Viewport& getViewport() const noexcept { return _viewport; }

    /// @return The world point under @p screen, in framebuffer pixels from the top-left corner.
    [[nodiscard]] glm::vec2 screenToWorld(glm::vec2 screen) const;
    /// @return Where @p world appears, in framebuffer pixels from the top-left corner.
    [[nodiscard]] glm::vec2 worldToScreen(glm::vec2 world) const;

  private:
    glm::mat4 _viewProjection{1.0F};  ///< World to clip space.
    glm::mat4 _inverse{1.0F};         ///< Clip space to world.
    Viewport _viewport;               ///< Letterboxed rectangle of the framebuffer.
};

}  // namespace rtype::plugins::backend::graphics
