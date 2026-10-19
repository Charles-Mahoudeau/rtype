/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputPlugin
*/

#include "InputPlugin.hpp"

#include <string>
#include <string_view>

#include "plugins/backend/event/PlatformEvent.hpp"
#include "plugins/input/Input.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::input {
namespace {
/// @brief Feeds the platform's events of the frame to Input, in the order they happened, then updates its actions.
void updateInput(ecs::ResMut<Input> input, ecs::EventReader<backend::event::PlatformEvent> events) {
    Input& state = *input;
    for (const backend::event::PlatformEvent& platformEvent : events) {
        state.handleEvent(platformEvent.event);
    }
    state.update();
}
}  // namespace

std::string_view InputPlugin::getName() const noexcept { return kName; }

void InputPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getEvents().registerType<backend::event::PlatformEvent>());
    app.insertResource(Input{});
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kUpdateSystem}, &updateInput).setSide(ecs::Side::kClient);
}
}  // namespace rtype::plugins::input
