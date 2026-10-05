/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IRenderer
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_uint2.hpp>
#include <span>

#include "engine/Export.hpp"
#include "engine/graphics/Camera.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/RectShape.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/platform/IPlatform.hpp"

namespace rtype::engine::graphics {

/// @brief 2D rendering, implemented by a graphics backend (Vulkan for now; SFML, SDL or raylib would fit too).
///
/// @details Kept at the level every 2D library offers: textures, a camera, sprites and rectangles. How they reach
/// the screen (batching, pipelines...) is up to the backend. Conventions, the ones of those libraries:
/// - World units, X to the right and Y down; angles in radians, clockwise on screen.
/// - Shapes are drawn in call order: the last one drawn is on top, so sort by layer before drawing.
///
/// A frame looks like:
/// @code
/// renderer.beginFrame(Color{0.0F, 0.0F, 0.0F, 1.0F});
/// renderer.setCamera(camera);
/// renderer.draw(background);
/// renderer.draw(ship);
/// renderer.endFrame();
/// @endcode
class RTYPE_ENGINE_API IRenderer {
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
    virtual void prepare(platform::IPlatform& /*platform*/) {}

    /// @brief Attaches the renderer to the window. Called once by the engine, after IPlatform::init().
    /// @details Takes what it needs from the platform: its framebuffer size, and whatever its graphics API needs
    /// through an interop interface the platform implements (Vulkan: interop::vulkan::IVulkanSurfaceSource).
    /// @throws exceptions::UnsupportedFeatureException If the platform does not provide what this renderer needs.
    virtual void init(platform::IPlatform& platform) = 0;

    /// @brief Adapts to a new framebuffer size. Called by the engine on every Resized event.
    virtual void resize(glm::uvec2 framebufferSize) = 0;

    /// @name Textures
    /// @brief Prefer the Texture class, which calls these and destroys the texture for you.
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

    /// @brief Sets the camera the next shapes are seen through.
    /// @details Until it is called, a frame shows the world in window pixels, origin at the top-left corner.
    virtual void setCamera(const Camera& camera) = 0;

    virtual void draw(const Sprite& sprite) = 0;
    virtual void draw(const RectShape& rect) = 0;
    /// @}
};
}  // namespace rtype::engine::graphics
