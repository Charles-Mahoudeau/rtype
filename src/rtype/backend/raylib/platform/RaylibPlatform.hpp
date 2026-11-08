/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RaylibPlatform
*/

#pragma once

#include <array>
#include <cstddef>
#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "backend/raylib/Export.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::backend::raylib::platform {

/// @brief The window and the input devices, through raylib, registered as the platform "raylib".
///
/// @details raylib opens its window and its OpenGL context together (InitWindow()), so this platform only works with
/// the renderer "raylib" (RaylibRenderer), which draws in that context. raylib reads the input devices when a frame
/// ends (EndDrawing()); pollEvents() turns the states it read into engine events by comparing them with the previous
/// frame. One gamepad, the first one, is reported.
class RTYPE_BACKEND_RAYLIB_API RaylibPlatform final : public contract::platform::IPlatform {
  public:
    RaylibPlatform() = default;
    /// @brief Closes the window, if init() opened it.
    ~RaylibPlatform() override;
    RaylibPlatform(const RaylibPlatform&) = delete;
    RaylibPlatform& operator=(const RaylibPlatform&) = delete;
    RaylibPlatform(RaylibPlatform&&) = delete;
    RaylibPlatform& operator=(RaylibPlatform&&) = delete;

    [[nodiscard]] glm::uvec2 getFramebufferSize() const override;
    [[nodiscard]] const std::string& getTitle() const override;
    void setTitle(const std::string& title) override;
    void setSize(glm::uvec2 size) override;
    /// @throws std::runtime_error If called twice, or if raylib cannot open the window.
    void init(const contract::platform::WindowConfig& config) override;
    [[nodiscard]] std::vector<contract::platform::Event> pollEvents() override;
    [[nodiscard]] bool isOpen() const noexcept override;
    void close() override;
    void setCursorLocked(bool locked) override;
    [[nodiscard]] double getTime() const override;
    void setClipboardText(std::string_view text) override;
    [[nodiscard]] std::string getClipboardText() const override;

    /// @return True once init() opened the window: what the renderer checks before drawing.
    [[nodiscard]] bool isWindowOpened() const noexcept { return _initialized; }

  private:
    /// @brief Appends the events of the gamepad: connection, buttons and axes that changed.
    void pollGamepad(std::vector<contract::platform::Event>& events);

    bool _initialized{false};  ///< True once InitWindow() succeeded.
    bool _isOpen{false};       ///< True between init() and the window closing.
    bool _focused{true};       ///< The focus at the last poll.
    bool _gamepad{false};      ///< A gamepad was connected at the last poll.
    std::array<float, static_cast<std::size_t>(contract::platform::input::GamepadAxis::kCount)>
        _axes{};         ///< The gamepad axes at the last poll.
    std::string _title;  ///< Window title.
};

}  // namespace rtype::backend::raylib::platform
