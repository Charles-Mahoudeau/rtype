/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldAccess
*/

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::view {
/// @brief Reads a field of a component's bytes as a Luau value.
///
/// @details Booleans give a boolean, integers and floats a number, an entity an `Entity` userdata. Vectors and asset
/// ids are not supported yet, and neither are array fields.
/// @param component Name of the component, for the messages.
/// @param field The field to read.
/// @param bytes The component's value.
/// @param entities The `Entity` usertype of the runtime, to build the userdata of an entity field.
/// @return The value, or an ErrorKind::kInvalidValue error naming the field.
[[nodiscard]] RTYPE_SCRIPTING_API Result<luau::Value> readField(std::string_view component, const ecs::FieldInfo& field,
                                                                std::span<const std::byte> bytes,
                                                                const luau::Usertype<ecs::Entity>& entities);

/// @brief Writes a Luau value into a field of a component's bytes.
///
/// @details Nothing is converted silently: a boolean field takes a boolean, a number field a number, an entity field
/// an `Entity`. An integer field takes a whole number inside its type's range; for `i64` and `u64` that range is
/// limited to +-2^53, the integers a Luau number holds exactly.
/// @param component Name of the component, for the messages.
/// @param field The field to write.
/// @param bytes The component's value.
/// @param value The new value.
/// @return Nothing on success, or an ErrorKind::kInvalidValue error naming the field (`nil`, a wrong type, a number
/// out of range or not whole, an unsupported field type).
[[nodiscard]] RTYPE_SCRIPTING_API Result<void> writeField(std::string_view component, const ecs::FieldInfo& field,
                                                          std::span<std::byte> bytes, const luau::Value& value);

/// @brief Writes the fields a Luau table names into a component's bytes, leaving the others as they are.
///
/// @details Used to build a component from its defaults and from the table of a `spawn`: @p bytes starts as the
/// default value.
/// @param type The component's description.
/// @param table Field names and their values.
/// @param bytes The component's value.
/// @return Nothing on success, or an ErrorKind::kInvalidValue error (a key that is not a field name, or a value
/// writeField() refuses).
[[nodiscard]] RTYPE_SCRIPTING_API Result<void> fillFromTable(const ecs::TypeInfo& type, const luau::Table& table,
                                                             std::span<std::byte> bytes);
}  // namespace rtype::scripting::view
