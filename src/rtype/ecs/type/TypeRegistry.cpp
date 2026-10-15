/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeRegistry
*/

#include "TypeRegistry.hpp"

#include <format>
#include <string_view>
#include <tl/expected.hpp>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
namespace {
[[nodiscard]] std::string_view toString(Presence presence) noexcept {
    switch (presence) {
        case Presence::kBoth:
            return "both";
        case Presence::kServerOnly:
            return "serverOnly";
        case Presence::kClientOnly:
            return "clientOnly";
    }
    return "unknown";
}

[[nodiscard]] std::string_view toString(ReplicationMode replication) noexcept {
    switch (replication) {
        case ReplicationMode::kNone:
            return "none";
        case ReplicationMode::kEveryChange:
            return "everyChange";
        case ReplicationMode::kSpawnOnly:
            return "spawnOnly";
    }
    return "unknown";
}
}  // namespace

namespace detail {
Error nameCollision(std::string_view label, std::string_view registered, std::string_view requested) {
    return Error{ErrorKind::kNameCollision,
                 std::format("{}s '{}' and '{}' have the same name hash", label, registered, requested)};
}

Error layoutMismatch(std::string_view label, std::string_view name) {
    return Error{ErrorKind::kLayoutMismatch,
                 std::format("{} '{}' is already registered with another layout (size, alignment, hooks or fields)",
                             label, name)};
}

Error optionsMismatch(std::string_view label, std::string_view name, std::string_view option,
                      std::string_view registered, std::string_view requested) {
    return Error{ErrorKind::kOptionsMismatch, std::format("{} '{}' is already registered with {} '{}', not '{}'", label,
                                                          name, option, registered, requested)};
}

Error noIdLeft(std::string_view label, std::string_view name) {
    return Error{ErrorKind::kInvalidType, std::format("{} '{}': no id left", label, name)};
}

Result<void> checkSameType(std::string_view label, const TypeInfo& registered, const TypeInfo& requested) {
    if (registered.name != requested.name) {
        return tl::unexpected<Error>{nameCollision(label, registered.name, requested.name)};
    }
    if (registered.layoutHash != requested.layoutHash) {
        return tl::unexpected<Error>{layoutMismatch(label, requested.name)};
    }
    if (registered.presence != requested.presence) {
        return tl::unexpected<Error>{optionsMismatch(label, requested.name, "presence", toString(registered.presence),
                                                     toString(requested.presence))};
    }
    if (registered.replication != requested.replication) {
        return tl::unexpected<Error>{optionsMismatch(
            label, requested.name, "replication", toString(registered.replication), toString(requested.replication))};
    }
    return {};
}
}  // namespace detail
}  // namespace rtype::ecs
