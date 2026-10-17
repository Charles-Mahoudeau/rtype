/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRegistry
*/

#pragma once

#include <string_view>

#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeRegistry.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
/// @brief On which side of the network a C++ event exists, chosen when it is registered. Events are never
/// replicated, so there is no replication mode.
struct EventOptions {
    Presence presence{Presence::kBoth};  ///< On which side of the network it exists.
};

/// @brief What the registry needs to know about events.
template <>
struct KindTraits<EventInfo> {
    using Options = EventOptions;                        ///< Presence.
    static constexpr TypeKind kKind = TypeKind::kEvent;  ///< Declared with RTYPE_ECS_EVENT.
    static constexpr std::string_view kName = "event";   ///< The kind in messages.
    static constexpr std::string_view kLabel = "Event";  ///< The kind at the start of a message.

    /// @return The rules of every type, and an event is never replicated; an event may be move-only.
    [[nodiscard]] static Result<void> validate(const EventInfo& info) { return rtype::ecs::validate(info); }

    /// @return Always nothing: events have no option of their own to compare.
    [[nodiscard]] static Result<void> checkSameOptions(const EventInfo& /*registered*/,
                                                       const EventInfo& /*requested*/) {
        return {};
    }

    /// @return The description of the C++ event `T` with these options.
    template <Event T>
    [[nodiscard]] static EventInfo make(const EventOptions& options);
};

/// @brief The catalog of the event types of one world: a TypeRegistry of EventInfo.
/// @code
/// rtype::ecs::EventRegistry registry;
/// const rtype::ecs::EventId collision = registry.registerType<Collision>();
/// const rtype::ecs::EventInfo* info = registry.findByName("rtype.Collision");
/// @endcode
using EventRegistry = TypeRegistry<EventInfo>;
}  // namespace rtype::ecs

#include "EventRegistry.tpp"
