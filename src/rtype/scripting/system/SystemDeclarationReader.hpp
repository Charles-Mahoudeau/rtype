/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemDeclarationReader
*/

#pragma once

#include <string>

#include "SystemDeclaration.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::system {
/// @brief Reads the table a script gives to `engine.ecs.system` as a SystemDeclaration.
///
/// @details The table has a `phase` (the name of an ecs::Phase, "FixedUpdate"), a `query` table, a `run` function, and
/// optionally `resources`, `events`, `after` and `before`, lists of system names. `query` holds lists of component
/// names under `read`, `write`, `optionalRead`, `optionalWrite`, `with`, `without`, `added` and `changed`; a Luau table
/// has no key order, so the terms come out in that order, each list in the order the script wrote it. `added` and
/// `changed` only filter the entities, like `with`: they give no view, so a script that needs the value lists the
/// component under `read` too. `resources` holds lists of resource names under `read` and `write` (a name in both is
/// written), and `events` lists of event names under `read` and `send` (a name in both is read and sent): they declare
/// what `ctx.resources` and `ctx.events` give the system, and what the scheduler sees of it. Nothing is guessed:
/// another key, a missing `phase`, `query` or `run`, an unknown phase or a list with anything but strings is an error.
/// @param name Name of the system, "game.ShieldRegen".
/// @param spec The declaration table.
/// @return The declaration, or an ErrorKind::kInvalidDeclaration error naming the system and what is wrong.
[[nodiscard]] RTYPE_SCRIPTING_API Result<SystemDeclaration> readSystemDeclaration(std::string name,
                                                                                  const luau::Table& spec);
}  // namespace rtype::scripting::system
