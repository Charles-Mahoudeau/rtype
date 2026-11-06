/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowEvents
*/

#pragma once

#include "plugins/backend/event/Event.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

// Every alternative of plugins::backend::Event declared as an ECS event, one queue each: the BackendPlugin sends them
// after polling the platform, and gameplay code and plugins (InputPlugin, the physics debug view) read them. Part of
// the contract, so a plugin reads them without knowing the backend plugin. The declaring macro must sit in the
// namespace of the type, so this reopens the engine's. The input ones are not reflected: they never reach Luau or the
// network, which see actions instead.
namespace rtype::plugins::backend::event {
RTYPE_ECS_EVENT(Closed, "rtype.Closed");
RTYPE_ECS_EVENT(Resized, "rtype.Resized");
RTYPE_ECS_EVENT(TextEntered, "rtype.TextEntered");
RTYPE_ECS_EVENT(KeyPressed, "rtype.KeyPressed");
RTYPE_ECS_EVENT(KeyReleased, "rtype.KeyReleased");
RTYPE_ECS_EVENT(MouseButtonPressed, "rtype.MouseButtonPressed");
RTYPE_ECS_EVENT(MouseButtonReleased, "rtype.MouseButtonReleased");
RTYPE_ECS_EVENT(MouseMoved, "rtype.MouseMoved");
RTYPE_ECS_EVENT(MouseScrolled, "rtype.MouseScrolled");
RTYPE_ECS_EVENT(FocusChanged, "rtype.FocusChanged");
RTYPE_ECS_EVENT(GamepadConnected, "rtype.GamepadConnected");
RTYPE_ECS_EVENT(GamepadDisconnected, "rtype.GamepadDisconnected");
RTYPE_ECS_EVENT(GamepadButtonPressed, "rtype.GamepadButtonPressed");
RTYPE_ECS_EVENT(GamepadButtonReleased, "rtype.GamepadButtonReleased");
RTYPE_ECS_EVENT(GamepadAxisMoved, "rtype.GamepadAxisMoved");
}  // namespace rtype::plugins::backend::event
