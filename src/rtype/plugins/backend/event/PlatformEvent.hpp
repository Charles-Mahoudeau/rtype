/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlatformEvent
*/

#pragma once

#include "plugins/backend/event/Event.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

namespace rtype::plugins::backend::event {
/// @brief Any event of the platform, as one ECS event: the BackendPlugin sends one per event polled, in the order the
/// platform produced them.
/// @details Readers that need the exact order of a frame's events (Input: a key released then pressed again within
/// one frame) read this queue. Gameplay code that wants one kind only reads its typed queue instead
/// (`EventReader<KeyPressed>`...), filled from the same events. Its field is not reflected: a variant is not.
struct PlatformEvent {
    Event event;  ///< The platform's event.
};
RTYPE_ECS_EVENT(PlatformEvent, "rtype.PlatformEvent");
}  // namespace rtype::plugins::backend::event
