/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentInfo
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief Dense index of a component type, local to one world. Use the component's name across worlds.
using ComponentId = TypeId;

/// @brief The id of no component.
inline constexpr ComponentId kInvalidComponentId = kInvalidTypeId;

/// @brief Where a component's data lives. Chosen per component.
enum class StorageKind : std::uint8_t {
    kSparseSet,  ///< One packed pool per component: cheap add and remove.
    kTable,      ///< Archetype column: fast multi-component iteration (v3; stored as a sparse set until then).
};

/// @brief Everything the ECS knows about a component type, whether it comes from C++ or from a Luau script.
///
/// @details What every kind of ECS data shares (name, layout, hooks, fields, network options) is in `type`; only
/// where the values live is specific to components. Storage, queries, reflection and replication only ever read this
/// description, which is what lets the core handle components that have no C++ type. Check a description with
/// validate() before using it.
struct ComponentInfo {
    TypeInfo type;                                 ///< Name, layout, hooks, fields and network options.
    StorageKind storage{StorageKind::kSparseSet};  ///< Where the values live.
};

/// @brief Checks that a component description is consistent: the rules of every type (validateType()), and a
/// component must be copyable (prefabs and snapshots copy it).
/// @return Nothing on success, or an ErrorKind::kInvalidComponent error naming the component and the problem.
[[nodiscard]] RTYPE_ECS_API Result<void> validate(const ComponentInfo& info);
}  // namespace rtype::ecs
