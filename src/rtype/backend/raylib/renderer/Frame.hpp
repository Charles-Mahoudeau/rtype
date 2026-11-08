/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Frame
*/

#pragma once

#include <raylib.h>

#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>

#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Overlay.hpp"
#include "rtype/contract/renderer/view/View.hpp"

namespace rtype::backend::raylib::renderer {

/// @brief The state of the frame being drawn: where the view is (a raylib camera and the letterboxed viewport the
/// engine's CameraView computes, so it matches the Vulkan renderer), the blend mode in use, and the spaces drawings are
/// placed in: the world (the view's camera, clipped to the viewport), the overlay (view pixels) and the screen
/// (framebuffer pixels, for tool meshes).
class Frame {
  public:
    /// @param virtualResolution The resolution the game is designed for; nothing: the window's.
    explicit Frame(std::optional<glm::uvec2> virtualResolution);

    /// @return The resolution the game is designed for, or 0x0 when the view is the window itself.
    [[nodiscard]] glm::vec2 getViewSize() const;

    /// @return True between begin() and end().
    [[nodiscard]] bool isDrawing() const noexcept { return _drawing; }

    /// @brief Starts a frame: black bars, @p clearColor in the viewport, the default view, alpha blending.
    void begin(const contract::renderer::Color& clearColor);

    /// @brief Ends the frame and shows it.
    void end();

    /// @brief Places the world under @p view, from now on.
    void setView(const contract::renderer::View& view);

    /// @brief Switches the blend mode, flushing raylib's batch only when it changes.
    void useBlend(contract::renderer::BlendMode blend);

    /// @brief Calls @p draw for each item, in view pixels from the viewport's top-left corner, each one inside its
    /// clip.
    void forEachOverlayItem(std::span<const contract::renderer::OverlayItem> items,
                            const std::function<void(const contract::renderer::OverlayItem&)>& draw);

    /// @brief Calls @p draw in framebuffer pixels, outside the view's camera and clip.
    void inScreenSpace(const std::function<void()>& draw);

  private:
    /// @brief Computes the camera and the viewport of @p view.
    void place(const contract::renderer::View& view);
    /// @brief Starts drawing with the camera, clipped to the viewport.
    void beginCamera() const;
    /// @brief Stops drawing with the camera.
    static void endCamera();

    std::optional<glm::uvec2> _virtualResolution;  ///< The game's resolution, or the window's.
    ::Camera2D _camera{};                          ///< The camera in use.
    ::Rectangle _viewport{};                       ///< The clip in use, in framebuffer pixels.
    float _fit{1.0F};                              ///< Framebuffer pixels per view pixel.
    int _blend{BLEND_ALPHA};                       ///< The blend mode in use.
    bool _drawing{false};                          ///< Between begin() and end().
};

}  // namespace rtype::backend::raylib::renderer
