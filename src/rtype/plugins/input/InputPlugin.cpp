/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputPlugin
*/

#include "InputPlugin.hpp"

#include <cstddef>
#include <string>
#include <string_view>

#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputEvents.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::input {
namespace {
/// @brief Feeds every event a reader took to Input.
template <typename Reader>
void feed(Input& input, const Reader& reader) {
    for (const auto& event : reader) {
        input.handleEvent(plugins::backend::Event{event});
    }
}

/// @brief Feeds an event without data to Input once per event taken: such events are counted, not read.
template <typename Event, typename Reader>
void feedEmpty(Input& input, const Reader& reader) {
    for (std::size_t index = 0; index < reader.size(); ++index) {
        input.handleEvent(plugins::backend::Event{Event{}});
    }
}

/// @brief Feeds the input events of the frame to Input, then updates its actions. Losing the focus comes first, so
/// what is pressed after it in the same frame still counts.
void updateInput(ecs::ResMut<Input> input, ecs::EventReader<plugins::backend::event::FocusChanged> focus,
                 ecs::EventReader<plugins::backend::event::KeyPressed> keysPressed,
                 ecs::EventReader<plugins::backend::event::KeyReleased> keysReleased,
                 ecs::EventReader<plugins::backend::event::MouseButtonPressed> buttonsPressed,
                 ecs::EventReader<plugins::backend::event::MouseButtonReleased> buttonsReleased,
                 ecs::EventReader<plugins::backend::event::MouseMoved> mouseMoved,
                 ecs::EventReader<plugins::backend::event::MouseScrolled> mouseScrolled,
                 ecs::EventReader<plugins::backend::event::GamepadConnected> gamepadConnected,
                 ecs::EventReader<plugins::backend::event::GamepadButtonPressed> gamepadPressed,
                 ecs::EventReader<plugins::backend::event::GamepadButtonReleased> gamepadReleased,
                 ecs::EventReader<plugins::backend::event::GamepadAxisMoved> gamepadAxes,
                 ecs::EventReader<plugins::backend::event::GamepadDisconnected> gamepadDisconnected) {
    Input& state = *input;
    feed(state, focus);
    feed(state, keysPressed);
    feed(state, keysReleased);
    feed(state, buttonsPressed);
    feed(state, buttonsReleased);
    feed(state, mouseMoved);
    feed(state, mouseScrolled);
    feedEmpty<plugins::backend::event::GamepadConnected>(state, gamepadConnected);
    feed(state, gamepadPressed);
    feed(state, gamepadReleased);
    feed(state, gamepadAxes);
    feedEmpty<plugins::backend::event::GamepadDisconnected>(state, gamepadDisconnected);
    state.update();
}

/// @brief Registers the event `T`, so the window plugin and the tests can send it before anything reads it.
template <typename T>
void registerEvent(ecs::Events& events) {
    static_cast<void>(events.registerType<T>());
}
}  // namespace

std::string_view InputPlugin::getName() const noexcept { return kName; }

void InputPlugin::build(ecs::App& app) {
    ecs::Events& events = app.getWorld().getEvents();
    registerEvent<plugins::backend::event::KeyPressed>(events);
    registerEvent<plugins::backend::event::KeyReleased>(events);
    registerEvent<plugins::backend::event::MouseButtonPressed>(events);
    registerEvent<plugins::backend::event::MouseButtonReleased>(events);
    registerEvent<plugins::backend::event::MouseMoved>(events);
    registerEvent<plugins::backend::event::MouseScrolled>(events);
    registerEvent<plugins::backend::event::FocusChanged>(events);
    registerEvent<plugins::backend::event::GamepadConnected>(events);
    registerEvent<plugins::backend::event::GamepadDisconnected>(events);
    registerEvent<plugins::backend::event::GamepadButtonPressed>(events);
    registerEvent<plugins::backend::event::GamepadButtonReleased>(events);
    registerEvent<plugins::backend::event::GamepadAxisMoved>(events);
    app.insertResource(Input{});
    ecs::Schedule::SystemConfig update =
        app.addSystem(ecs::Phase::kPreUpdate, std::string{kUpdateSystem}, &updateInput).setSide(ecs::Side::kClient);
    if (app.hasPlugin(backend::BackendPlugin::kName)) {
        update.after(std::string{backend::BackendPlugin::kEventsSystem});
    }
}
}  // namespace rtype::plugins::input
