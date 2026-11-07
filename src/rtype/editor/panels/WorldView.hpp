/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldView
*/

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "editor/Export.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::editor::panels {

/// @return The name of a component or a resource without its namespace: "Transform" for "rtype.Transform".
[[nodiscard]] RTYPE_EDITOR_API std::string shortName(std::string_view name);

/// @return The entities of @p world that have a component, by index: only those with a component whose name contains
/// @p filter (case ignored) when it is not empty. An entity without any component is not in the list, as the world
/// keeps no list of entities of its own.
[[nodiscard]] RTYPE_EDITOR_API std::vector<ecs::Entity> collectEntities(const ecs::World& world,
                                                                        std::string_view filter);

/// @return A line that tells @p entity apart: its index and its first components' names.
[[nodiscard]] RTYPE_EDITOR_API std::string describeEntity(const ecs::World& world, ecs::Entity entity);

}  // namespace rtype::editor::panels
