/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Controls
*/

#pragma once

#include "plugins/input/Input.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/input/InputLayout.hpp"

// Client systems: the local controls, the camera and the way out.

namespace rtype_duel {

/// @brief Startup: binds the local controls: W / A / S / D or the arrows to move, the left mouse button to fire.
void bindControls(rtype::ecs::ResMut<rtype::plugins::input::Input> input);

/// @brief Startup: spawns the camera, centred on the arena, so the whole 800x600 view is shown; the sounds are panned
/// from it.
void spawnCamera(rtype::ecs::Commands& commands);

/// @brief PostUpdate, before the input is sent: copies the local controls into the network input.
void sendControls(rtype::ecs::Res<rtype::plugins::input::Input> input,
                  rtype::ecs::Res<rtype::replication::input::InputLayout> layout,
                  rtype::ecs::ResMut<rtype::replication::client::ClientState> client);

/// @brief Update: stops the App when Escape is pressed.
void quitOnEscape(rtype::ecs::Res<rtype::plugins::input::Input> input,
                  rtype::ecs::EventWriter<rtype::ecs::AppExit> exit);

}  // namespace rtype_duel
