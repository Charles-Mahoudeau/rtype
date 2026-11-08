/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RaylibRenderer
*/

#include "RaylibRenderer.hpp"

#include <raylib.h>

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <variant>

#include "Frame.hpp"
#include "TextureTable.hpp"
#include "backend/raylib/platform/RaylibPlatform.hpp"
#include "backend/raylib/renderer/draw/Meshes.hpp"
#include "backend/raylib/renderer/draw/MsdfText.hpp"
#include "backend/raylib/renderer/draw/Particles.hpp"
#include "backend/raylib/renderer/draw/Shapes.hpp"
#include "backend/raylib/renderer/draw/Sprites.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/exceptions/PlatformExceptions.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"
#include "rtype/contract/renderer/drawing/Overlay.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "rtype/contract/renderer/view/View.hpp"

namespace rtype::backend::raylib::renderer {

namespace cr = contract::renderer;

struct RaylibRenderer::Parts {
    explicit Parts(std::optional<glm::uvec2> virtualResolution) : frame(virtualResolution) {}

    Frame frame;            ///< The view, the blend mode and the drawing spaces.
    TextureTable textures;  ///< The textures by id.
    draw::MsdfText text;    ///< The text shader.
};

RaylibRenderer::RaylibRenderer(std::optional<glm::uvec2> virtualResolution)
    : _parts(std::make_unique<Parts>(virtualResolution)) {}

RaylibRenderer::~RaylibRenderer() = default;

void RaylibRenderer::init(contract::platform::IPlatform& platform) {
    const auto* raylibPlatform = dynamic_cast<const platform::RaylibPlatform*>(&platform);
    if (raylibPlatform == nullptr || !raylibPlatform->isWindowOpened()) {
        throw std::runtime_error(R"(The renderer "raylib" needs the platform "raylib" (engine.settings.platform))");
    }
    _parts->text.load();
}

void RaylibRenderer::resize(glm::uvec2 /*framebufferSize*/) {}

glm::vec2 RaylibRenderer::getViewSize() const { return _parts->frame.getViewSize(); }

cr::TextureId RaylibRenderer::createTexture(const cr::TextureDesc& desc, std::span<const std::byte> pixels) {
    return _parts->textures.create(desc, pixels);
}

void RaylibRenderer::destroy(cr::TextureId texture) { _parts->textures.destroy(texture); }

void RaylibRenderer::beginFrame(const cr::Color& clearColor) { _parts->frame.begin(clearColor); }

void RaylibRenderer::endFrame() { _parts->frame.end(); }

void RaylibRenderer::setView(const cr::View& view) { _parts->frame.setView(view); }

void RaylibRenderer::draw(const cr::Sprite& sprite) {
    if (const ::Texture2D* texture = _parts->textures.find(sprite.texture); texture != nullptr) {
        _parts->frame.useBlend(sprite.blend);
        draw::drawSprite(*texture, sprite);
    }
}

void RaylibRenderer::draw(const cr::Shape& shape) {
    _parts->frame.useBlend(shape.blend);
    draw::drawShape(shape);
}

void RaylibRenderer::draw(const cr::Text& text) {
    if (const ::Texture2D* atlas = _parts->textures.find(text.texture); atlas != nullptr && !text.glyphs.empty()) {
        _parts->frame.useBlend(text.blend);
        _parts->text.draw(*atlas, text);
    }
}

void RaylibRenderer::drawOverlay(std::span<const cr::OverlayItem> items) {
    _parts->frame.forEachOverlayItem(items, [this](const cr::OverlayItem& item) {
        std::visit([this](const auto& drawing) { draw(drawing); }, item.drawing);
    });
}

cr::ParticleSystemId RaylibRenderer::createParticleSystem(const cr::ParticleSystemDesc& /*desc*/) {
    throw contract::platform::exceptions::UnsupportedFeatureException(
        "The raylib renderer has no GPU particles: the particle plugin simulates them on the CPU");
}

void RaylibRenderer::destroy(cr::ParticleSystemId /*system*/) {}

void RaylibRenderer::simulate(cr::ParticleSystemId /*system*/, const cr::ParticleEmission& /*emission*/) {
    throw contract::platform::exceptions::UnsupportedFeatureException(
        "The raylib renderer has no GPU particles: the particle plugin simulates them on the CPU");
}

void RaylibRenderer::draw(const cr::ParticleDraw& particles) {
    const auto* instances = std::get_if<std::span<const cr::ParticleInstance>>(&particles.particles);
    const ::Texture2D* texture = _parts->textures.find(particles.texture);
    if (instances != nullptr && texture != nullptr) {
        _parts->frame.useBlend(particles.blend);
        draw::drawParticles(*texture, *instances, particles);
    }
}

void RaylibRenderer::drawMesh(const cr::Mesh& mesh) {
    if (mesh.commands.empty()) {
        return;
    }
    _parts->frame.inScreenSpace([this, &mesh] {
        _parts->frame.useBlend(cr::BlendMode::kAlpha);
        draw::drawMesh(mesh, _parts->textures);
    });
}

}  // namespace rtype::backend::raylib::renderer
