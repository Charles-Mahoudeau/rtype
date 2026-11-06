/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DeclarationReader
*/

#pragma once

#include <string>

#include "ComponentDeclaration.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::component {
/// @brief Reads the table a script gives to `engine.ecs.component` as a ComponentDeclaration.
///
/// @details The table has a `fields` table of field names and type names (`bool`, `i8`..`i64`, `u8`..`u64`, `f32`,
/// `f64`, `entity`), and optionally `defaults`, field names and values written like a field is, and `replication`
/// (`"none"`, `"everyChange"` or `"spawnOnly"`) and `presence` (`"both"`, `"serverOnly"` or `"clientOnly"`, `"both"`
/// by default). Nothing is guessed: another key, an unknown or not yet supported field
/// type, a default for a field that does not exist or that its type refuses is an error. Fields come out sorted by
/// name, since a Luau table has no order.
/// @param name Name of the component, "game.Shield".
/// @param spec The declaration table.
/// @return The declaration, or an ErrorKind::kInvalidDeclaration error naming the component and what is wrong, or an
/// ErrorKind::kInvalidValue one when a default does not fit its field.
[[nodiscard]] RTYPE_SCRIPTING_API Result<ComponentDeclaration> readComponentDeclaration(std::string name,
                                                                                        const luau::Table& spec);
}  // namespace rtype::scripting::component
