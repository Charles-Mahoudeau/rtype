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

/// @brief Types a component field can have, understood by reflection, scripting and replication.
enum class FieldType : std::uint8_t {
    kBool,     ///< `bool`.
    kI8,       ///< `std::int8_t`.
    kI16,      ///< `std::int16_t`.
    kI32,      ///< `std::int32_t`.
    kI64,      ///< `std::int64_t`.
    kU8,       ///< `std::uint8_t`.
    kU16,      ///< `std::uint16_t`.
    kU32,      ///< `std::uint32_t`.
    kU64,      ///< `std::uint64_t`.
    kF32,      ///< `float`.
    kF64,      ///< `double`.
    kVec2,     ///< Two floats (glm layout).
    kVec3,     ///< Three floats (glm layout).
    kVec4,     ///< Four floats (glm layout).
    kEntity,   ///< An Entity: remapped by the network, cleared when its target is released.
    kAssetId,  ///< A stable 64-bit asset identifier, the same in every process.
};

/// @brief Size and alignment of one value of a FieldType, in bytes.
struct FieldLayout {
    std::uint32_t size;       ///< Bytes taken by one value.
    std::uint32_t alignment;  ///< Required alignment of one value.
};

/// @return The size and alignment of one value of the given type.
[[nodiscard]] constexpr FieldLayout getFieldLayout(FieldType type) noexcept {
    switch (type) {
        case FieldType::kBool:
        case FieldType::kI8:
        case FieldType::kU8:
            return {.size = 1, .alignment = 1};
        case FieldType::kI16:
        case FieldType::kU16:
            return {.size = 2, .alignment = 2};
        case FieldType::kI32:
        case FieldType::kU32:
        case FieldType::kF32:
            return {.size = 4, .alignment = 4};
        case FieldType::kI64:
        case FieldType::kU64:
        case FieldType::kF64:
        case FieldType::kEntity:
        case FieldType::kAssetId:
            return {.size = 8, .alignment = 8};
        case FieldType::kVec2:
            return {.size = 8, .alignment = 4};
        case FieldType::kVec3:
            return {.size = 12, .alignment = 4};
        case FieldType::kVec4:
            return {.size = 16, .alignment = 4};
    }
    return {.size = 0, .alignment = 1};  // Unreachable: every FieldType is handled above.
}

/// @brief One reflected field of a component.
struct FieldInfo {
    std::string name;                  ///< Field name, unique within the component.
    FieldType type{FieldType::kBool};  ///< Type of each value.
    std::uint32_t offset{0};           ///< Byte offset of the first value inside the component.
    std::uint32_t count{1};            ///< Number of values: 1 for a scalar, N for a fixed array.
};

/// @brief Type-erased lifecycle of a component's bytes. A null hook means trivial: zero-fill, memcpy or nothing.
///
/// @details Hooks only manage the component's own memory (a C++ component holding a `std::vector`); gameplay
/// reactions belong in systems. They are null for plain data and always null for Luau components. Moving and
/// destroying cannot throw: their pointer types are `noexcept`.
struct ComponentHooks {
    void (*construct)(void* destination) = nullptr;                    ///< Default-constructs in raw storage.
    void (*destruct)(void* target) noexcept = nullptr;                 ///< Destroys a value.
    void (*move)(void* destination, void* source) noexcept = nullptr;  ///< Move-constructs into raw storage.
    void (*copy)(void* destination, const void* source) = nullptr;     ///< Copy-constructs into raw storage.
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
    ComponentHooks hooks;                                 ///< Lifecycle of the bytes; null when trivial.
    std::vector<FieldInfo> fields;                        ///< Reflected fields.
    std::uint64_t layoutHash{0};                          ///< Hash of the name and fields, set by the registry.
};

/// @brief Checks that a component description is consistent.
/// @return Nothing on success, or an ErrorKind::kInvalidComponent error naming the component and the problem: an
/// empty name, a bad alignment or size, fields on a tag, a client-only replicated component, a replicated component
/// without fields, or a field that is unnamed, duplicated, empty, misaligned, out of bounds or overlapping another.
[[nodiscard]] RTYPE_ECS_API Result<void> validate(const ComponentInfo& info);
}  // namespace rtype::ecs
