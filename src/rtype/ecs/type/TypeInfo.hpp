/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeInfo
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeHooks.hpp"

namespace rtype::ecs {
/// @brief Dense index of a type in its registry, local to one world. Use the type's name across worlds.
using TypeId = std::uint32_t;

/// @brief The id of no type.
inline constexpr TypeId kInvalidTypeId = std::numeric_limits<TypeId>::max();

/// @brief On which side of the network a type's values exist.
enum class Presence : std::uint8_t {
    kBoth,        ///< On the server and on the clients.
    kServerOnly,  ///< Never on a client: AI state, secrets.
    kClientOnly,  ///< Never on the server: interpolation, effects, render state.
};

/// @brief How the server sends a type's values to the clients.
enum class ReplicationMode : std::uint8_t {
    kNone,         ///< Never sent.
    kEveryChange,  ///< Sent whenever it changed since the client's last acknowledged tick.
    kSpawnOnly,    ///< Sent once when it appears; clients simulate the rest (bullets).
};

/// @brief What every kind of ECS data (components, resources) knows about its type, whether it comes from C++ or
/// from a Luau script: enough to store, copy, move, destroy, reflect and replicate its values without a C++ type.
///
/// @details Each kind wraps it with what is specific to it (a component's storage, a resource's thread). Check a
/// description with validateType() before using it.
struct TypeInfo {
    TypeId id{kInvalidTypeId};                            ///< Dense id, assigned by the registry.
    std::string name;                                     ///< Stable, unique key: "rtype.Position", "rtype.Time".
    std::size_t size{0};                                  ///< Bytes per value; 0 for an empty type (a tag).
    std::size_t alignment{1};                             ///< Required alignment, a power of two.
    Presence presence{Presence::kBoth};                   ///< On which side of the network it exists.
    ReplicationMode replication{ReplicationMode::kNone};  ///< How the server sends it.
    TypeHooks hooks;                                      ///< Lifecycle of the bytes; null when trivial.
    bool copyable{true};                                  ///< Whether a value can be copied (false for move-only).
    std::vector<FieldInfo> fields;                        ///< Reflected fields.
    std::uint64_t layoutHash{0};  ///< computeLayoutHash() of this description, set by the registry.
};

/// @brief Checks the rules every kind of type follows.
/// @param kind The kind of the type, lowercase ("component"), used in the messages.
/// @return Nothing on success, or an ErrorKind::kInvalidType error naming the type and the problem: an empty
/// name, a bad alignment or size, fields on an empty type, copyable without a copy hook, a client-only replicated
/// type, a replicated type without fields, or a field that is unnamed, duplicated, empty, misaligned, out of bounds or
/// overlapping another.
[[nodiscard]] RTYPE_ECS_API Result<void> validateType(const TypeInfo& info, std::string_view kind);

/// @brief Fingerprint of a type's layout: its name, size, alignment, which lifecycle hooks it has, whether it is
/// copyable, and every field (name, type, offset, count). Presence and replication are not part of it.
///
/// @details Two processes that compute the same hash agree on the type's bytes: the registry uses it to accept
/// re-registering an identical type, and the network compares it when a client connects.
[[nodiscard]] RTYPE_ECS_API std::uint64_t computeLayoutHash(const TypeInfo& info) noexcept;
}  // namespace rtype::ecs
