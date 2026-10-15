/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentInfo
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeHooks.hpp"

namespace rtype::ecs {
/// @brief Dense index of a component type, local to one world. Use the component's name across worlds.
using ComponentId = std::uint32_t;

/// @brief The id of no component.
inline constexpr ComponentId kInvalidComponentId = std::numeric_limits<ComponentId>::max();

/// @brief Where a component's data lives. Chosen per component.
enum class StorageKind : std::uint8_t {
    kSparseSet,  ///< One packed pool per component: cheap add and remove.
    kTable,      ///< Archetype column: fast multi-component iteration (v3; stored as a sparse set until then).
};

/// @brief On which side of the network the component exists.
enum class Presence : std::uint8_t {
    kBoth,        ///< On the server and on the clients.
    kServerOnly,  ///< Never on a client: AI state, secrets.
    kClientOnly,  ///< Never on the server: interpolation, effects, render state.
};

/// @brief How the server sends the component to the clients.
enum class ReplicationMode : std::uint8_t {
    kNone,         ///< Never sent.
    kEveryChange,  ///< Sent whenever it changed since the client's last acknowledged tick.
    kSpawnOnly,    ///< Sent once when the entity appears; clients simulate the rest (bullets).
};

/// @brief Everything the ECS knows about a component type, whether it comes from C++ or from a Luau script.
///
/// @details Storage, queries, reflection and replication only ever read this description, which is what lets the
/// core handle components that have no C++ type. Check a description with validate() before using it.
struct ComponentInfo {
    ComponentId id{kInvalidComponentId};                  ///< Dense id, assigned by the registry.
    std::string name;                                     ///< Stable, unique key: "rtype.Position", "game.Shield".
    std::size_t size{0};                                  ///< Bytes per value; 0 for a tag.
    std::size_t alignment{1};                             ///< Required alignment, a power of two.
    StorageKind storage{StorageKind::kSparseSet};         ///< Where the values live.
    Presence presence{Presence::kBoth};                   ///< On which side of the network it exists.
    ReplicationMode replication{ReplicationMode::kNone};  ///< How the server sends it.
    TypeHooks hooks;                                      ///< Lifecycle of the bytes; null when trivial.
    std::vector<FieldInfo> fields;                        ///< Reflected fields.
    std::uint64_t layoutHash{0};                          ///< Hash of the name and fields, set by the registry.
};

/// @brief Checks that a component description is consistent.
/// @return Nothing on success, or an ErrorKind::kInvalidComponent error naming the component and the problem: an
/// empty name, a bad alignment or size, fields on a tag, a client-only replicated component, a replicated component
/// without fields, or a field that is unnamed, duplicated, empty, misaligned, out of bounds or overlapping another.
[[nodiscard]] RTYPE_ECS_API Result<void> validate(const ComponentInfo& info);

/// @brief Fingerprint of a component's layout: its name, size, alignment, which lifecycle hooks it has, and every
/// field (name, type, offset, count). The options (storage, presence, replication) are not part of it.
///
/// @details Two processes that compute the same hash agree on the component's bytes: the registry uses it to accept
/// re-registering an identical component, and the network compares it when a client connects.
[[nodiscard]] RTYPE_ECS_API std::uint64_t computeLayoutHash(const ComponentInfo& info) noexcept;
}  // namespace rtype::ecs
