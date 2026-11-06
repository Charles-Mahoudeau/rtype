/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkInput
*/

#pragma once

#include <string_view>

#include "Export.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/plugins/input/Input.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::engine {
/// @brief The client system that fills the input sent to the server: PostUpdate, before the client plugin's
/// `net.sendInput`.
inline constexpr std::string_view kNetworkInputSystem = "rtype.engine.networkInput";

/// @brief Reads the player's input actions into a PlayerInput: each action of @p layout from the action of @p input
/// that has the same name, as the layout's kind (held for a button, the axis, the vector).
/// @details An action @p input does not have stays neutral.
/// @param input The input plugin's actions.
/// @param layout The network InputLayout.
/// @return The input to send; its sequence and previous buttons are left to the client.
[[nodiscard]] RTYPE_ENGINE_API replication::component::PlayerInput readNetworkInput(
    const plugins::input::Input& input, const replication::input::InputLayout& layout);

/// @brief Adds to a client App the system that fills the input sent to the server from the player's input actions, so
/// that a game sends its input without code of its own.
///
/// @details Every frame, it reads the actions of the network InputLayout (`net.InputLayout`, declared by
/// `engine.input.action` or InputLayout::add()) with readNetworkInput() and gives the result to
/// ClientState::setInput(). An empty layout sends nothing. A game that fills the input itself can still do it
/// in a system ordered after this one. The App must have the input and client plugins: the Engine calls it on clients,
/// after adding them.
/// @param app The client App.
RTYPE_ENGINE_API void addNetworkInput(ecs::App& app);
}  // namespace rtype::engine
