/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkInput
*/

#pragma once

#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string_view>

#include "Export.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::engine {
/// @brief The client system that fills the input sent to the server: PostUpdate, before the client plugin's
/// `net.sendInput`.
inline constexpr std::string_view kNetworkInputSystem = "rtype.engine.networkInput";

/// @brief The engine call the system reads the input actions through: the input plugin's `rtype.input.read`. The Engine
/// links no plugin, so this call, found by name in the world, is all it knows of the input plugin.
inline constexpr std::string_view kReadActionCall = "rtype.input.read";

/// @brief What an input action reads this frame.
struct ActionReading {
    bool held{false};       ///< Down this frame.
    glm::vec2 value{0.0F};  ///< Its value: the X alone for a button or an axis.
};

/// @brief Reads an input action by name.
/// @return What it reads, or nothing when there is no action of this name.
using ActionReader = std::function<std::optional<ActionReading>(std::string_view name)>;

/// @brief Reads the player's input actions into a PlayerInput: each action of @p layout from the action of the same
/// name, as the layout's kind (held for a button, the axis, the vector).
/// @details An action @p read does not have stays neutral.
/// @param layout The network InputLayout.
/// @param read Reads an input action by name.
/// @return The input to send; its sequence and previous buttons are left to the client.
[[nodiscard]] RTYPE_ENGINE_API replication::component::PlayerInput readNetworkInput(
    const replication::input::InputLayout& layout, const ActionReader& read);

/// @brief Adds to a client App the system that fills the input sent to the server from the player's input actions, so
/// that a game sends its input without code of its own.
///
/// @details Every frame, it reads the actions of the network InputLayout (`net.InputLayout`, declared by
/// `engine.input.action` or InputLayout::add()) through the `rtype.input.read` engine call with readNetworkInput(), and
/// gives the result to ClientState::setInput(). An empty layout sends nothing. A game that fills the input itself can
/// still do it in a system ordered after this one. The client plugin must have been added; an App without the input
/// plugin (no `rtype.input.read` call) gets no system and sends no input.
/// @param app The client App, with its plugins added.
/// @throws exceptions::RuntimeError When the input plugin's call does not have the arguments and the result the Engine
/// expects: the plugin library and the Engine come from different versions.
RTYPE_ENGINE_API void addNetworkInput(ecs::App& app);
}  // namespace rtype::engine
