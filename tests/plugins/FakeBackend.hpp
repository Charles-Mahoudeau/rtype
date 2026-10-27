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
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/View.hpp"
#include "plugins/backend/platform/IPlatform.hpp"
#include "plugins/backend/platform/WindowConfig.hpp"

namespace rtype::test::plugins {
/// @brief What the fake platform and renderer do and record, shared with the test that drives them.
struct FakeBackendState {
    std::vector<std::vector<rtype::plugins::backend::Event>> frames;  ///< The events each poll returns, in order.
    glm::uvec2 framebufferSize{800, 600};                             ///< 0x0 while minimized.
    bool open{true};                                                  ///< Cleared to close the window.
    int closeAfterPolls{-1};  ///< Closes the window after this many polls (-1: never).
    int initCount{0};         ///< platform.init() calls.
    int polls{0};             ///< pollEvents() calls.
    int waits{0};             ///< waitEvents() calls.
    int beginFrames{0};       ///< renderer.beginFrame() calls.
    int endFrames{0};         ///< renderer.endFrame() calls.
    rtype::plugins::backend::graphics::Color clearColor{0.0F};                    ///< The last clear colour.
    std::vector<glm::uvec2> resizes;                                              ///< Every renderer.resize() call.
    std::vector<rtype::plugins::backend::graphics::TextureDesc> createdTextures;  ///< Every createTexture() desc.
    std::vector<std::vector<std::byte>> createdPixels;                            ///< Every createTexture() pixels.
    std::vector<rtype::plugins::backend::graphics::TextureId> destroyedTextures;  ///< Every destroy() call.
    std::vector<rtype::plugins::backend::graphics::View> views;                   ///< Every setView() call.
    std::vector<rtype::plugins::backend::graphics::Sprite> sprites;               ///< Every draw(Sprite) call.
    std::vector<rtype::plugins::backend::graphics::Shape> shapes;                 ///< Every draw(Shape) call.
};

/// @brief A window that returns scripted events.
class FakePlatform : public rtype::plugins::backend::platform::IPlatform {
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
    void init(const rtype::plugins::backend::platform::WindowConfig& config) override {
        _title = config.title;
        ++_state->initCount;
    }
    [[nodiscard]] std::vector<rtype::plugins::backend::Event> pollEvents() override {
        ++_state->polls;
        return next();
    }
    [[nodiscard]] std::vector<rtype::plugins::backend::Event> waitEvents() override {
        ++_state->waits;
        return next();
    }
    [[nodiscard]] bool isOpen() const noexcept override { return _state->open; }
    void close() override { _state->open = false; }
    void setCursorLocked(bool /*locked*/) override {}
    [[nodiscard]] double getTime() const override { return 0.0; }

  private:
    /// @return The next scripted frame of events, closing the window when its time has come.
    std::vector<rtype::plugins::backend::Event> next() {
        const int calls = _state->polls + _state->waits;
        if (_state->closeAfterPolls >= 0 && calls >= _state->closeAfterPolls) {
            _state->open = false;
        }
        if (_state->frames.empty()) {
            return {};
        }
        std::vector<rtype::plugins::backend::Event> events = std::move(_state->frames.front());
        _state->frames.erase(_state->frames.begin());
        return events;
    }

    std::shared_ptr<FakeBackendState> _state;  ///< Shared with the test.
    std::string _title;                        ///< From init().
};

/// @brief A renderer that draws nothing and records the frames.
class FakeRenderer : public rtype::plugins::backend::graphics::IRenderer {
  public:
    explicit FakeRenderer(std::shared_ptr<FakeBackendState> state) : _state{std::move(state)} {}
    ~FakeRenderer() override = default;
    FakeRenderer(const FakeRenderer& other) = delete;
    FakeRenderer& operator=(const FakeRenderer& other) = delete;
    FakeRenderer(FakeRenderer&& other) = delete;
    FakeRenderer& operator=(FakeRenderer&& other) = delete;

    void init(rtype::plugins::backend::platform::IPlatform& /*platform*/) override {}
    void resize(glm::uvec2 framebufferSize) override { _state->resizes.push_back(framebufferSize); }
    [[nodiscard]] rtype::plugins::backend::graphics::TextureId createTexture(
        const rtype::plugins::backend::graphics::TextureDesc& desc, std::span<const std::byte> pixels) override {
        _state->createdTextures.push_back(desc);
        _state->createdPixels.emplace_back(pixels.begin(), pixels.end());
        return {.index = static_cast<std::uint32_t>(_state->createdTextures.size() - 1), .generation = 1};
    }
    void destroy(rtype::plugins::backend::graphics::TextureId texture) override {
        _state->destroyedTextures.push_back(texture);
    }
    void beginFrame(const rtype::plugins::backend::graphics::Color& clearColor) override {
        ++_state->beginFrames;
        _state->clearColor = clearColor;
    }
    void endFrame() override { ++_state->endFrames; }
    void setView(const rtype::plugins::backend::graphics::View& view) override { _state->views.push_back(view); }
    void draw(const rtype::plugins::backend::graphics::Sprite& sprite) override { _state->sprites.push_back(sprite); }
    void draw(const rtype::plugins::backend::graphics::Shape& shape) override { _state->shapes.push_back(shape); }

  private:
    std::shared_ptr<FakeBackendState> _state;  ///< Shared with the test.
};

/// @return A registry holding the fake platform and renderer as "fake", both driven by `state`.
inline std::unique_ptr<rtype::plugins::backend::BackendRegistry> makeFakeRegistry(
    const std::shared_ptr<FakeBackendState>& state) {
    auto registry = std::make_unique<rtype::plugins::backend::BackendRegistry>();
    registry->addPlatform("fake", [state](const rtype::core::config::Settings& /*settings*/) {
        return std::make_unique<FakePlatform>(state);
    });
    registry->addRenderer("fake", [state](const rtype::core::config::Settings& /*settings*/) {
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
}  // namespace rtype::test::plugins
