/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputForms
*/

#pragma once

#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/Runtime.hpp"

namespace rtype::scripting::setup {
/// @brief Builds `engine.input`, the form that declares an input action and its bindings while scripts load.
///
/// @details `action(name, type, {bind = {...}})` adds the action (`rtype.input.addAction`, `type` "button" by
/// default, "axis" or "vector2"), then each binding of `bind`, in order:
/// - a control name, `"Space"`: `rtype.input.bind`;
/// - `{control = "GamepadLeftY", scale = -1}`: `rtype.input.bind` with a scale;
/// - `{negative = "A", positive = "D"}`: `rtype.input.bindAxis`;
/// - `{up = "W", down = "S", left = "A", right = "D"}` or `{x = "GamepadLeftX", y = "GamepadLeftY"}`:
///   `rtype.input.bindVector`.
///
/// Where the replication plugin is added, it also declares the action in the network InputLayout
/// (`rtype.replication.addInput`, same name and type), on the server as on its clients, so that the server reads the
/// action from each ship's PlayerInput and the engine fills it on clients.
///
/// It returns nothing. It checks its whole table first, then skips its calls when the App does not have them (no input
/// plugin in this process, such as on a dedicated server; no replication plugin in a standalone game). Control names
/// are checked by the input plugin. An unknown key, a binding of no known shape or a failing call is a Luau error
/// naming the form.
/// @param runtime The scripts' runtime; it must outlive the library.
/// @param app The App the scripts set up; it must outlive the form's calls.
/// @return The `input` library.
[[nodiscard]] luau::Library buildInputForms(luau::Runtime& runtime, ecs::App& app);
}  // namespace rtype::scripting::setup
