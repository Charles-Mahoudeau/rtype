/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IRenderer
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <span>

#include "plugins/Export.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/View.hpp"
#include "plugins/backend/platform/IPlatform.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief 2D rendering, implemented by a graphics backend (Vulkan for now; SFML, SDL or raylib would fit too).
///
/// @details Kept at the level every 2D library offers: textures, a view, sprites and shapes (rectangles, circles,
/// capsules, lines). How they reach the screen (batching, pipelines...) is up to the backend. Conventions, the ones of
/// those libraries:
/// - World units, X to the right and Y down; angles in radians, clockwise on screen.
/// - Sprites are drawn in call order: the last one drawn is on top, so sort by layer before drawing. Shapes are
///   overlays, drawn in call order among themselves, on top of the sprites.
///
/// A frame looks like:
/// @code
/// renderer.beginFrame(Color{0.0F, 0.0F, 0.0F, 1.0F});
/// renderer.setView(view);
/// renderer.draw(background);
/// renderer.draw(ship);
/// renderer.endFrame();
/// @endcode
class RTYPE_PLUGINS_API IRenderer {
  public:
    IRenderer() = default;
    virtual ~IRenderer() = default;
    IRenderer(const IRenderer&) = delete;
    IRenderer& operator=(const IRenderer&) = delete;
    IRenderer(IRenderer&&) = delete;
    IRenderer& operator=(IRenderer&&) = delete;

    /// @brief Sets the platform up before its window exists. Called once by the engine, before IPlatform::init().
    /// @details The only moment to change how the platform will create its window (Vulkan: which loader GLFW
    /// uses). A renderer that can already tell the platform does not fit should throw here, before a window
    /// opens. Optional. Default: does nothing.
    /// @throws exceptions::UnsupportedFeatureException If the platform does not provide what this renderer needs.
    virtual void setup(platform::IPlatform& /*platform*/) {}

    /// @brief Attaches the renderer to the window. Called once by the engine, after IPlatform::init().
    /// @details Takes what it needs from the platform: its framebuffer size, and whatever its graphics API needs
    /// through an interop interface the platform implements (Vulkan: interop::vulkan::IVulkanSurfaceSource).
    /// @throws exceptions::UnsupportedFeatureException If the platform does not provide what this renderer needs.
    virtual void init(platform::IPlatform& platform) = 0;

    /// @brief Adapts to a new framebuffer size. Called by the engine on every Resized event.
    virtual void resize(glm::uvec2 framebufferSize) = 0;

    /// @return The resolution the game is designed for, which setView() fits in the window with bars (the viewSize of
    /// graphics::CameraView); 0x0 when the view is the framebuffer itself, one world unit per pixel at zoom 1.
    [[nodiscard]] virtual glm::vec2 getViewSize() const { return glm::vec2{0.0F}; }

    /// @name Textures
    /// @brief The ids own nothing: destroy() frees a texture early, and the renderer frees every texture left when it
    /// is destroyed. Games load textures through the TextureAssets resource, which also decodes image files.
    /// @{

    /// @param pixels RGBA pixels, 8 bits per channel, row by row from the top: desc.size.x * desc.size.y * 4 bytes.
    [[nodiscard]] virtual TextureId createTexture(const TextureDesc& desc, std::span<const std::byte> pixels) = 0;

    /// @brief Destroys a texture. Safe to call while the backend may still be drawing it.
    virtual void destroy(TextureId texture) = 0;
    /// @}

    /// @name Frame
    /// @{

    /// @brief Starts a frame, cleared with the given colour.
    virtual void beginFrame(const Color& clearColor) = 0;

    /// @brief Shows what was drawn since beginFrame().
    virtual void endFrame() = 0;
    /// @}

    /// @name Drawing (between beginFrame() and endFrame())
    /// @{

    /// @brief Sets the view the frame is seen through: call it once per frame, before drawing (a backend may apply
    /// the last view set to the whole frame, as Vulkan does). graphics::CameraView computes what it shows.
    /// @details Until it is called, a frame shows the world in window pixels, origin at the top-left corner.
    virtual void setView(const View& view) = 0;

    virtual void draw(const Sprite& sprite) = 0;
    /// @brief Draws a shape (rectangle, circle, capsule, line), filled and/or outlined.
    /// @details A backend may draw every shape of a frame after its sprites, as Vulkan does (one draw call for them
    /// all): shapes are for overlays (debug views, outlines, UI), never under a sprite.
    virtual void draw(const Shape& shape) = 0;
    /// @}
};
}  // namespace rtype::plugins::backend::graphics
