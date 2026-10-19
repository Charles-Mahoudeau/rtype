/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::input {
/// @brief Input for an App: the Input resource, kept up to date from the platform's input events.
///
/// @details Inserts Input, registers the input events (InputEvents.hpp) and adds a PreUpdate client system,
/// "rtype.input.update", that feeds them to Input and calls Input::update(). The events come from the window plugin,
/// which sends them after polling the platform; the InputPlugin never talks to a window, so it also runs with events
/// sent by hand (tests, replays). Actions are declared on the resource, then read by the gameplay systems:
/// @code
/// app.addPlugin(rtype::plugins::input::InputPlugin{});
/// app.getWorld().getResources().requireMut<rtype::plugins::input::Input>()->addAction(
///     "fire", rtype::plugins::input::ActionType::kButton);
///
/// void fire(rtype::ecs::Res<rtype::plugins::input::Input> input) {
///     if (input->getAction("fire").isPressed()) { ... }
/// }
/// @endcode
/// The events of one frame are fed by kind (keys, then mouse, then gamepad), not in the order they happened: a
/// button released then pressed again within one frame reads as released until its next press.
class InputPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.input";  ///< The plugin's name.

    InputPlugin() = default;
    ~InputPlugin() override = default;
    InputPlugin(const InputPlugin& other) = default;
    InputPlugin& operator=(const InputPlugin& other) = default;
    InputPlugin(InputPlugin&& other) noexcept = default;
    InputPlugin& operator=(InputPlugin&& other) noexcept = default;

    /// @return "rtype.input".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts Input, registers the input events and adds the system that updates Input from them.
    void build(ecs::App& app) override;
};
}  // namespace rtype::plugins::input
