/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsoleTypes
*/

#pragma once

#include <memory>

#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/view/ViewScope.hpp"

namespace rtype::scripting::console {
/// @brief What a debug console line is allowed to reach in the world: set by the tool that runs the line.
struct ConsoleAccess {
    ecs::World* world{nullptr};              ///< The world whose entities the line reads and writes.
    bool writable{true};                     ///< False: the components are read-only (a client of a server).
    std::shared_ptr<view::ViewScope> scope;  ///< Invalidates what the line made once it is over.
};

/// @brief Registers `ConsoleEntity` and `ConsoleComponent`, what a console line uses to reach the components of an
/// entity.
///
/// @details `entity.Ship` (the component's name, or its last part if that is not ambiguous; `entity["game.Ship"]` for
/// the whole name) gives a `ConsoleComponent`, or nil if the entity has none; `entity.Ship.speed` reads a field and
/// `entity.Ship.speed = 3` writes it, in place. Both are read at each access, so nothing dangles inside a line, and
/// they are valid for one line (the scope moves on after it). `entity.__keys` and `component.__keys` list the names to
/// complete. Must be called before the runtime is sealed, after view::registerViewTypes().
/// @param runtime The runtime to register in; it must not move afterwards.
/// @return Nothing, or the error of the registration.
[[nodiscard]] RTYPE_SCRIPTING_API luau::Result<> registerConsoleTypes(luau::Runtime& runtime);

/// @brief Makes the `ConsoleEntity` of @p entity for the line @p access describes.
/// @return The userdata, or the error of the creation.
[[nodiscard]] RTYPE_SCRIPTING_API luau::Result<luau::Value> makeConsoleEntity(const luau::Runtime& runtime,
                                                                              const ConsoleAccess& access,
                                                                              ecs::Entity entity);
}  // namespace rtype::scripting::console
