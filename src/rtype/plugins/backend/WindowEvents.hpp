/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowEvents
*/

#pragma once

#include "engine/event/Event.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

// The window events of engine::Event that are not input, declared as ECS events: the BackendPlugin sends them after
// polling the platform (the input ones are in plugins/input/InputEvents.hpp). The declaring macro must sit in the
// namespace of the type, so this reopens the engine's.
namespace rtype::engine::event {
RTYPE_ECS_EVENT(Closed, "rtype.Closed");
RTYPE_ECS_EVENT(Resized, "rtype.Resized");
RTYPE_ECS_EVENT(TextEntered, "rtype.TextEntered");
}  // namespace rtype::engine::event
