/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FakeBackend
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/shape/ShapePlugin.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/exceptions/PlatformExceptions.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/contract/renderer/view/View.hpp"
#include "rtype/ecs/app/App.hpp"

namespace rtype::test::plugins {
/// @brief What the fake platform and renderer do and record, shared with the test that drives them.
struct FakeBackendState {
    std::vector<std::vector<rtype::contract::platform::Event>> frames;  ///< The events each poll returns, in order.
    glm::uvec2 framebufferSize{800, 600};                               ///< 0x0 while minimized.
    bool open{true};                                                    ///< Cleared to close the window.
    int closeAfterPolls{-1};                            ///< Closes the window after this many polls (-1: never).
    int initCount{0};                                   ///< platform.init() calls.
    int polls{0};                                       ///< pollEvents() calls.
    int waits{0};                                       ///< waitEvents() calls.
    int beginFrames{0};                                 ///< renderer.beginFrame() calls.
    int endFrames{0};                                   ///< renderer.endFrame() calls.
    rtype::contract::renderer::Color clearColor{0.0F};  ///< The last clear colour.
    std::vector<glm::uvec2> resizes;                    ///< Every renderer.resize() call.
    std::vector<rtype::contract::renderer::TextureDesc> createdTextures;  ///< Every createTexture() desc.
    std::vector<std::vector<std::byte>> createdPixels;                    ///< Every createTexture() pixels.
    std::vector<rtype::contract::renderer::TextureId> destroyedTextures;  ///< Every destroy() call.
    std::vector<rtype::contract::renderer::View> views;                   ///< Every setView() call.
    std::vector<rtype::contract::renderer::Sprite> sprites;               ///< Every draw(Sprite) call.
    std::vector<rtype::contract::renderer::Shape> shapes;                 ///< Every draw(Shape) call.
    std::vector<rtype::contract::renderer::Text> texts;                   ///< Every draw(Text) call.
    bool gpuParticles{false};                                             ///< What supportsGpuParticles() returns.
    std::vector<rtype::contract::renderer::ParticleSystemDesc> createdParticleSystems;  ///< Every create call.
    std::vector<rtype::contract::renderer::ParticleSystemId> destroyedParticleSystems;  ///< Every destroy() call.
    std::vector<rtype::contract::renderer::ParticleEmission> simulations;               ///< Every simulate() call.
    std::vector<rtype::contract::renderer::ParticleDraw> particleDraws;     ///< Every draw(ParticleDraw) call.
    bool shaders{false};                                                    ///< What supportsShaders() returns.
    std::vector<rtype::contract::renderer::CompiledShader> createdShaders;  ///< Every createShader() call.
    std::vector<rtype::contract::renderer::ShaderId> destroyedShaders;      ///< Every destroy(ShaderId) call.
    std::vector<rtype::contract::renderer::MaterialDesc> createdMaterials;  ///< Every createMaterial() call.
    std::vector<rtype::contract::renderer::MaterialDesc> updatedMaterials;  ///< Every updateMaterial() call.
    std::vector<rtype::contract::renderer::MaterialId> destroyedMaterials;  ///< Every destroy(MaterialId).
    std::vector<char> drawOrder;  ///< 's' (sprite), 'h' (shape), 't' (text) or 'p' (particles) per draw() call.
    std::vector<rtype::contract::renderer::Mesh> meshes;       ///< Every drawMesh() call.
    std::vector<std::optional<glm::uvec2>> sceneSizeRequests;  ///< Every setSceneSize() call.
    std::optional<glm::uvec2> sceneSize;                       ///< What getSceneSize() returns: the last request.
    rtype::contract::renderer::TextureId sceneTexture;         ///< What getSceneTexture() returns.
};

/// @brief A window that returns scripted events.
class FakePlatform : public rtype::contract::platform::IPlatform {
  public:
    explicit FakePlatform(std::shared_ptr<FakeBackendState> state) : _state{std::move(state)} {}
    ~FakePlatform() override = default;
    FakePlatform(const FakePlatform& other) = delete;
    FakePlatform& operator=(const FakePlatform& other) = delete;
    FakePlatform(FakePlatform&& other) = delete;
    FakePlatform& operator=(FakePlatform&& other) = delete;

    [[nodiscard]] glm::uvec2 getFramebufferSize() const override { return _state->framebufferSize; }
    [[nodiscard]] const std::string& getTitle() const override { return _title; }
    void setTitle(const std::string& title) override { _title = title; }
    void setSize(glm::uvec2 /*size*/) override {}
    void init(const rtype::contract::platform::WindowConfig& config) override {
        _title = config.title;
        ++_state->initCount;
    }
    [[nodiscard]] std::vector<rtype::contract::platform::Event> pollEvents() override {
        ++_state->polls;
        return next();
    }
    [[nodiscard]] std::vector<rtype::contract::platform::Event> waitEvents() override {
        ++_state->waits;
        return next();
    }
    [[nodiscard]] bool isOpen() const noexcept override { return _state->open; }
    void close() override { _state->open = false; }
    void setCursorLocked(bool /*locked*/) override {}
    [[nodiscard]] double getTime() const override { return 0.0; }

  private:
    /// @return The next scripted frame of events, closing the window when its time has come.
    std::vector<rtype::contract::platform::Event> next() {
        const int calls = _state->polls + _state->waits;
        if (_state->closeAfterPolls >= 0 && calls >= _state->closeAfterPolls) {
            _state->open = false;
        }
        if (_state->frames.empty()) {
            return {};
        }
        std::vector<rtype::contract::platform::Event> events = std::move(_state->frames.front());
        _state->frames.erase(_state->frames.begin());
        return events;
    }

    std::shared_ptr<FakeBackendState> _state;  ///< Shared with the test.
    std::string _title;                        ///< From init().
};

/// @brief A renderer that draws nothing and records the frames.
class FakeRenderer : public rtype::contract::renderer::IRenderer {
  public:
    explicit FakeRenderer(std::shared_ptr<FakeBackendState> state) : _state{std::move(state)} {}
    ~FakeRenderer() override = default;
    FakeRenderer(const FakeRenderer& other) = delete;
    FakeRenderer& operator=(const FakeRenderer& other) = delete;
    FakeRenderer(FakeRenderer&& other) = delete;
    FakeRenderer& operator=(FakeRenderer&& other) = delete;

    void init(rtype::contract::platform::IPlatform& /*platform*/) override {}
    void resize(glm::uvec2 framebufferSize) override { _state->resizes.push_back(framebufferSize); }
    [[nodiscard]] rtype::contract::renderer::TextureId createTexture(const rtype::contract::renderer::TextureDesc& desc,
                                                                     std::span<const std::byte> pixels) override {
        _state->createdTextures.push_back(desc);
        _state->createdPixels.emplace_back(pixels.begin(), pixels.end());
        return {.index = static_cast<std::uint32_t>(_state->createdTextures.size() - 1), .generation = 1};
    }
    void destroy(rtype::contract::renderer::TextureId texture) override {
        _state->destroyedTextures.push_back(texture);
    }
    void beginFrame(const rtype::contract::renderer::Color& clearColor) override {
        ++_state->beginFrames;
        _state->clearColor = clearColor;
    }
    void endFrame() override { ++_state->endFrames; }
    void setView(const rtype::contract::renderer::View& view) override { _state->views.push_back(view); }
    void draw(const rtype::contract::renderer::Sprite& sprite) override {
        _state->sprites.push_back(sprite);
        _state->drawOrder.push_back('s');
    }
    void draw(const rtype::contract::renderer::Shape& shape) override {
        _state->shapes.push_back(shape);
        _state->drawOrder.push_back('h');
    }
    void draw(const rtype::contract::renderer::Text& text) override {
        _state->texts.push_back(text);
        _state->drawOrder.push_back('t');
    }
    [[nodiscard]] bool supportsGpuParticles() const noexcept override { return _state->gpuParticles; }
    [[nodiscard]] rtype::contract::renderer::ParticleSystemId createParticleSystem(
        const rtype::contract::renderer::ParticleSystemDesc& desc) override {
        if (!_state->gpuParticles) {
            throw rtype::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no GPU particles");
        }
        _state->createdParticleSystems.push_back(desc);
        return {.index = static_cast<std::uint32_t>(_state->createdParticleSystems.size() - 1), .generation = 1};
    }
    void destroy(rtype::contract::renderer::ParticleSystemId system) override {
        _state->destroyedParticleSystems.push_back(system);
    }
    void simulate(rtype::contract::renderer::ParticleSystemId /*system*/,
                  const rtype::contract::renderer::ParticleEmission& emission) override {
        _state->simulations.push_back(emission);
    }
    // Copies the draw: the CPU instances it points to are only valid until the frame is drawn.
    void draw(const rtype::contract::renderer::ParticleDraw& particles) override {
        _state->particleDraws.push_back(particles);
        _state->drawOrder.push_back('p');
    }
    [[nodiscard]] bool supportsShaders() const noexcept override { return _state->shaders; }
    [[nodiscard]] rtype::contract::renderer::ShaderId createShader(
        const rtype::contract::renderer::CompiledShader& shader) override {
        if (!_state->shaders) {
            throw rtype::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no shaders");
        }
        rtype::contract::renderer::validate(shader);
        _state->createdShaders.push_back(shader);
        return {.index = static_cast<std::uint32_t>(_state->createdShaders.size() - 1), .generation = 1};
    }
    void destroy(rtype::contract::renderer::ShaderId shader) override { _state->destroyedShaders.push_back(shader); }
    [[nodiscard]] rtype::contract::renderer::MaterialId createMaterial(
        const rtype::contract::renderer::MaterialDesc& desc) override {
        if (!_state->shaders) {
            throw rtype::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no shaders");
        }
        _state->createdMaterials.push_back(desc);
        return {.index = static_cast<std::uint32_t>(_state->createdMaterials.size() - 1), .generation = 1};
    }
    void updateMaterial(rtype::contract::renderer::MaterialId /*material*/,
                        const rtype::contract::renderer::MaterialDesc& desc) override {
        _state->updatedMaterials.push_back(desc);
    }
    void destroy(rtype::contract::renderer::MaterialId material) override {
        _state->destroyedMaterials.push_back(material);
    }
    void drawMesh(const rtype::contract::renderer::Mesh& mesh) override { _state->meshes.push_back(mesh); }
    void setSceneSize(std::optional<glm::uvec2> size) override {
        _state->sceneSizeRequests.push_back(size);
        _state->sceneSize = size;
    }
    [[nodiscard]] std::optional<glm::uvec2> getSceneSize() const override { return _state->sceneSize; }
    [[nodiscard]] rtype::contract::renderer::TextureId getSceneTexture() const override { return _state->sceneTexture; }

  private:
    std::shared_ptr<FakeBackendState> _state;  ///< Shared with the test.
};

/// @return A registry holding the fake platform and renderer as "fake", both driven by `state`.
inline std::unique_ptr<rtype::plugins::graphics::BackendRegistry> makeFakeRegistry(
    const std::shared_ptr<FakeBackendState>& state) {
    auto registry = std::make_unique<rtype::plugins::graphics::BackendRegistry>();
    registry->platforms.add("fake", [state](const rtype::core::config::Settings& /*settings*/) {
        return std::make_unique<FakePlatform>(state);
    });
    registry->renderers.add("fake", [state](const rtype::core::config::Settings& /*settings*/) {
        return std::make_unique<FakeRenderer>(state);
    });
    return registry;
}

/// @return Settings naming the fake platform and renderer.
inline rtype::core::config::Settings makeFakeSettings() {
    rtype::core::config::Settings settings;
    settings.set("platform", "fake");
    settings.set("renderer", "fake");
    settings.set("window.title", "Test");
    return settings;
}
/// @brief Adds the GraphicsPlugin and every plugin that draws, as a client does.
inline void addDrawingPlugins(rtype::ecs::App& app, std::unique_ptr<rtype::plugins::graphics::BackendRegistry> registry,
                              rtype::core::config::Settings settings) {
    app.addPlugin(rtype::plugins::transform::TransformPlugin{});
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{std::move(registry), std::move(settings)});
    app.addPlugin(rtype::plugins::shader::ShaderPlugin{});
    app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    app.addPlugin(rtype::plugins::sprite::SpriteDrawPlugin{});
    app.addPlugin(rtype::plugins::shape::ShapePlugin{});
    app.addPlugin(rtype::plugins::tilemap::TilemapPlugin{});
    app.addPlugin(rtype::plugins::text::TextPlugin{});
    app.addPlugin(rtype::plugins::particle::ParticlePlugin{});
}

}  // namespace rtype::test::plugins
