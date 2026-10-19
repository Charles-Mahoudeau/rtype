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

#include "engine/event/Event.hpp"
#include "plugins/backend/BackendPlugin.hpp"
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
        input.handleEvent(engine::Event{event});
    }
}

/// @brief Feeds an event without data to Input once per event taken: such events are counted, not read.
template <typename Event, typename Reader>
void feedEmpty(Input& input, const Reader& reader) {
    for (std::size_t index = 0; index < reader.size(); ++index) {
        input.handleEvent(engine::Event{Event{}});
    }
}

/// @brief Feeds the input events of the frame to Input, then updates its actions. Losing the focus comes first, so
/// what is pressed after it in the same frame still counts.
void updateInput(ecs::ResMut<Input> input, ecs::EventReader<engine::event::FocusChanged> focus,
                 ecs::EventReader<engine::event::KeyPressed> keysPressed,
                 ecs::EventReader<engine::event::KeyReleased> keysReleased,
                 ecs::EventReader<engine::event::MouseButtonPressed> buttonsPressed,
                 ecs::EventReader<engine::event::MouseButtonReleased> buttonsReleased,
                 ecs::EventReader<engine::event::MouseMoved> mouseMoved,
                 ecs::EventReader<engine::event::MouseScrolled> mouseScrolled,
                 ecs::EventReader<engine::event::GamepadConnected> gamepadConnected,
                 ecs::EventReader<engine::event::GamepadButtonPressed> gamepadPressed,
                 ecs::EventReader<engine::event::GamepadButtonReleased> gamepadReleased,
                 ecs::EventReader<engine::event::GamepadAxisMoved> gamepadAxes,
                 ecs::EventReader<engine::event::GamepadDisconnected> gamepadDisconnected) {
    Input& state = *input;
    feed(state, focus);
    feed(state, keysPressed);
    feed(state, keysReleased);
    feed(state, buttonsPressed);
    feed(state, buttonsReleased);
    feed(state, mouseMoved);
    feed(state, mouseScrolled);
    feedEmpty<engine::event::GamepadConnected>(state, gamepadConnected);
    feed(state, gamepadPressed);
    feed(state, gamepadReleased);
    feed(state, gamepadAxes);
    feedEmpty<engine::event::GamepadDisconnected>(state, gamepadDisconnected);
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
    registerEvent<engine::event::KeyPressed>(events);
    registerEvent<engine::event::KeyReleased>(events);
    registerEvent<engine::event::MouseButtonPressed>(events);
    registerEvent<engine::event::MouseButtonReleased>(events);
    registerEvent<engine::event::MouseMoved>(events);
    registerEvent<engine::event::MouseScrolled>(events);
    registerEvent<engine::event::FocusChanged>(events);
    registerEvent<engine::event::GamepadConnected>(events);
    registerEvent<engine::event::GamepadDisconnected>(events);
    registerEvent<engine::event::GamepadButtonPressed>(events);
    registerEvent<engine::event::GamepadButtonReleased>(events);
    registerEvent<engine::event::GamepadAxisMoved>(events);
    app.insertResource(Input{});
    ecs::Schedule::SystemConfig update =
        app.addSystem(ecs::Phase::kPreUpdate, std::string{kUpdateSystem}, &updateInput).setSide(ecs::Side::kClient);
    // The backend sends the events Input reads, in the same phase. A constraint naming a missing system is an
    // error, so it is set by whichever of the two plugins is added second.
    if (app.hasPlugin(backend::BackendPlugin::kName)) {
        update.after(std::string{backend::BackendPlugin::kEventsSystem});
    }
}
}  // namespace rtype::plugins::input
