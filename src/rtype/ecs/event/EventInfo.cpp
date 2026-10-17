/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventInfo
*/

#include "EventInfo.hpp"

#include <format>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
Result<void> validate(const EventInfo& info) {
    if (Result<void> valid = validateType(info.type, "event"); !valid) {
        return valid;
    }
    if (info.type.replication != ReplicationMode::kNone) {
        return fail(ErrorKind::kInvalidType,
                    std::format("Event '{}': an event is local to a world and cannot be replicated", info.type.name));
    }
    return {};
}
}  // namespace rtype::ecs
