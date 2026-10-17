/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventInfo
*/

#pragma once

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief Dense index of an event type, local to one world. Use the event's name across worlds.
using EventId = TypeId;

/// @brief The id of no event.
inline constexpr EventId kInvalidEventId = kInvalidTypeId;

/// @brief Everything the ECS knows about an event type: a message sent between systems that do not know each other
/// (a collision, a window resize), whether it comes from C++ or from a Luau script.
///
/// @details What every kind of ECS data shares (name, layout, hooks, copyability, fields, presence) is in `type`;
/// events have nothing of their own yet. Events are local to a world: they are never replicated. Like a resource, an
/// event may be any class, including a move-only one. Check a description with validate() before using it.
struct EventInfo {
    TypeInfo type;  ///< Name, layout, hooks, copyability, fields and presence.
};

/// @brief Checks that an event description is consistent: the rules of every type (validateType()), and an event
/// is never replicated. An event may be move-only.
/// @return Nothing on success, or an ErrorKind::kInvalidType error naming the event and the problem.
[[nodiscard]] RTYPE_ECS_API Result<void> validate(const EventInfo& info);
}  // namespace rtype::ecs
