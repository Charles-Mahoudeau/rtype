/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SfmlPlatform
*/

#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <string>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/platform/IPlatform.hpp"
#include "engine/platform/WindowConfig.hpp"

namespace example {

/// @brief An IPlatform written with SFML 3: the window and its events.
///
/// @details With SFML, the window and the renderer are one object (sf::RenderWindow). This platform owns it, and
/// the matching SfmlRenderer draws into it through getWindow(): the two are a pair, created together. It implements
/// no interop interface (such as IVulkanSurfaceSource): SfmlRenderer needs none.
///
/// Kept short on purpose: only the common keys and the mouse are translated, and gamepads are ignored.
class SfmlPlatform final : public rtype::engine::platform::IPlatform {
  public:
    /// @brief Does not open anything yet: the engine calls init().
    SfmlPlatform() = default;
    ~SfmlPlatform() override = default;
    SfmlPlatform(const SfmlPlatform&) = delete;
    SfmlPlatform& operator=(const SfmlPlatform&) = delete;
    SfmlPlatform(SfmlPlatform&&) = delete;
    SfmlPlatform& operator=(SfmlPlatform&&) = delete;

    [[nodiscard]] glm::uvec2 getFramebufferSize() const override;
    [[nodiscard]] const std::string& getTitle() const override;
    void setTitle(const std::string& title) override;
    void setSize(glm::uvec2 size) override;

    /// @throws std::runtime_error If init() was already called.
    void init(const rtype::engine::platform::WindowConfig& config) override;

    [[nodiscard]] std::vector<rtype::engine::Event> pollEvents() override;
    [[nodiscard]] bool isOpen() const noexcept override;
    void close() override;
    void setCursorLocked(bool locked) override;
    [[nodiscard]] double getTime() const override;

    /// @return The window, for SfmlRenderer to draw into.
    /// @throws std::runtime_error If init() was not called yet.
    [[nodiscard]] sf::RenderWindow& getWindow();

  private:
    std::unique_ptr<sf::RenderWindow> _window;  ///< The SFML window, created by init().
    bool _isOpen{false};                        ///< True between init() and the window closing.
    std::string _title;                         ///< Window title.
    sf::Clock _clock;                           ///< Started by init(), for getTime().
    glm::vec2 _cursorPosition{0.0F};            ///< Last cursor position, to compute MouseMoved::delta.
    bool _hasCursorPosition{false};             ///< False until the first cursor event, to avoid a first jump.
};
}  // namespace example
