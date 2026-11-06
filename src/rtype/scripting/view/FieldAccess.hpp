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

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::view {
/// @brief What reading a field needs to build the values it gives to scripts.
struct FieldReadContext {
    const luau::Runtime* runtime{nullptr};          ///< The runtime scripts run in, to build tables; must not move.
    luau::Usertype<ecs::Entity> entities;           ///< The `Entity` usertype of the runtime.
    luau::Usertype<core::asset::AssetId> assetIds;  ///< The `AssetId` usertype of the runtime.
};

/// @brief Reads a field of a component's bytes as a Luau value.
///
/// @details Booleans give a boolean, integers and floats a number, an entity an `Entity` userdata, an asset id an
/// `AssetId` userdata. A `vec3` gives a Luau `vector`, a `vec2` a table `{x=, y=}` and a `vec4` a table
/// `{x=, y=, z=, w=}`, since the Luau build has three-wide vectors only. Array fields are not supported.
/// @param component Name of the component, for the messages.
/// @param field The field to read.
/// @param bytes The component's value.
/// @param context The runtime and usertypes to build the values with.
/// @return The value, or an ErrorKind::kInvalidValue error naming the field.
[[nodiscard]] RTYPE_SCRIPTING_API Result<luau::Value> readField(std::string_view component, const ecs::FieldInfo& field,
                                                                std::span<const std::byte> bytes,
                                                                const FieldReadContext& context);

/// @brief Writes a Luau value into a field of a component's bytes.
///
/// @details Nothing is converted silently: a boolean field takes a boolean, a number field a number, an entity field
/// an `Entity`. A `vec3` field takes a `vector` or a table `{x=, y=, z=}`, a `vec2` and a `vec4` field a table with
/// all their keys (`x`, `y`, and `z`, `w`), and no other key. An asset id field takes a path string, which is hashed
/// to its `AssetId`, or an `AssetId`. An integer field takes a whole number inside its type's range; for `i64` and
/// `u64` that range is limited to +-2^53, the integers a Luau number holds exactly.
/// @param component Name of the component, for the messages.
/// @param field The field to write.
/// @param bytes The component's value.
/// @param value The new value.
/// @return Nothing on success, or an ErrorKind::kInvalidValue error naming the field (`nil`, a wrong type, a number
/// out of range or not whole, a vector table with a missing, extra or non-number key, an unsupported field type).
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
