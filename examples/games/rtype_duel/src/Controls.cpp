/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Controls
*/

#include "Controls.hpp"

#include <glm/ext/vector_float2.hpp>
#include <string>

#include "Constants.hpp"
#include "plugins/audio/AudioListener.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/transform/Transform.hpp"

using namespace rtype;

namespace rtype_duel {

void bindControls(ecs::ResMut<plugins::input::Input> input) {
    using plugins::backend::input::Key;
    using plugins::input::Control;
    input->addAction(std::string{kMoveAction}, plugins::input::ActionType::kVector2)
        .bindVector(Control::key(Key::kW), Control::key(Key::kS), Control::key(Key::kA), Control::key(Key::kD))
        .bindVector(Control::key(Key::kUp), Control::key(Key::kDown), Control::key(Key::kLeft),
                    Control::key(Key::kRight));
    input->addAction(std::string{kShootAction}, plugins::input::ActionType::kButton)
        .bind(Control::mouseButton(plugins::backend::input::MouseButton::kLeft));
}

void spawnCamera(ecs::Commands& commands) {
    commands.spawn()
        .add(plugins::transform::Transform{.position = {kViewWidth / 2.0F, kViewHeight / 2.0F}})
        .add(plugins::backend::graphics::Camera{})
        .add(plugins::audio::AudioListener{});
}

void sendControls(ecs::Res<plugins::input::Input> input, ecs::Res<replication::input::InputLayout> layout,
                  ecs::ResMut<replication::client::ClientState> client) {
    replication::component::PlayerInput sent;
    const glm::vec2 move = input->getAction(std::string{kMoveAction}).readVector();
    layout->setVector(sent, kMoveAction, {move.x, move.y});
    layout->setButton(sent, kShootAction, input->getAction(std::string{kShootAction}).isHeld());
    client->setInput(sent);
}

void quitOnEscape(ecs::Res<plugins::input::Input> input, ecs::EventWriter<ecs::AppExit> exit) {
    if (input->read(plugins::input::Control::key(plugins::backend::input::Key::kEscape)) > 0.0F) {
        exit.send(ecs::AppExit{.code = 0});
    }
}

}  // namespace rtype_duel
