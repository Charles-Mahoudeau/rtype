/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RaylibRenderer
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>

#include "backend/raylib/Export.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"

namespace rtype::backend::raylib::renderer {

/// @brief A 2D renderer on raylib, registered as the renderer "raylib": textures, sprites, shapes, MSDF text, particles
/// simulated by the engine on the CPU, the overlay and tool meshes.
///
/// @details It draws in the window and the OpenGL context of the platform "raylib", the only one it accepts. It only
/// dispatches: Frame keeps the view and the blend mode, TextureTable the textures, and the functions of draw/ turn each
/// drawing into raylib calls. What raylib's OpenGL 3.3 path cannot do is reported through the contract's capability
/// checks: no GPU particles (the particle plugin then simulates them on the CPU) and no user shaders (SPIR-V).
class RTYPE_BACKEND_RAYLIB_API RaylibRenderer final : public contract::renderer::IRenderer {
  public:
    /// @param virtualResolution The resolution the game is designed for, fitted in the window with bars; nothing: the
    /// window itself.
    explicit RaylibRenderer(std::optional<glm::uvec2> virtualResolution);
    ~RaylibRenderer() override;
    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;
    RaylibRenderer(RaylibRenderer&&) = delete;
    RaylibRenderer& operator=(RaylibRenderer&&) = delete;

    /// @throws std::runtime_error If @p platform is not the platform "raylib", or its window is not open.
    void init(contract::platform::IPlatform& platform) override;
    void resize(glm::uvec2 framebufferSize) override;
    [[nodiscard]] glm::vec2 getViewSize() const override;

    [[nodiscard]] contract::renderer::TextureId createTexture(const contract::renderer::TextureDesc& desc,
                                                              std::span<const std::byte> pixels) override;
    void destroy(contract::renderer::TextureId texture) override;

    void beginFrame(const contract::renderer::Color& clearColor) override;
    void endFrame() override;
    void setView(const contract::renderer::View& view) override;
    void draw(const contract::renderer::Sprite& sprite) override;
    void draw(const contract::renderer::Shape& shape) override;
    void draw(const contract::renderer::Text& text) override;
    void drawOverlay(std::span<const contract::renderer::OverlayItem> items) override;

    /// @throws contract::platform::exceptions::UnsupportedFeatureException Always: particles run on the CPU.
    [[nodiscard]] contract::renderer::ParticleSystemId createParticleSystem(
        const contract::renderer::ParticleSystemDesc& desc) override;
    void destroy(contract::renderer::ParticleSystemId system) override;
    /// @throws contract::platform::exceptions::UnsupportedFeatureException Always: particles run on the CPU.
    void simulate(contract::renderer::ParticleSystemId system,
                  const contract::renderer::ParticleEmission& emission) override;
    void draw(const contract::renderer::ParticleDraw& particles) override;

    void drawMesh(const contract::renderer::Mesh& mesh) override;

  private:
    struct Parts;  ///< Frame, textures and text shader: they hold raylib types, kept out of this header.

    std::unique_ptr<Parts> _parts;  ///< Never null.
};

}  // namespace rtype::backend::raylib::renderer
