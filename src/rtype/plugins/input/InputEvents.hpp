/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputEvents
*/

#pragma once

#include "plugins/backend/event/Event.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

// The platform events Input reads, declared as ECS events: one queue per alternative of plugins::backend::Event. The
// window plugin sends them after polling the platform; the InputPlugin reads them. The declaring macro must sit in the
// namespace of the type, so this reopens the engine's. Their fields are not reflected: they never reach Luau or the
// network, which see actions instead.
namespace rtype::plugins::backend::event {
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
