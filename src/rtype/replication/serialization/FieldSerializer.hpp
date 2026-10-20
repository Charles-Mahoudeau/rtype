/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldSerializer
*/

#pragma once

#include <cstddef>
#include <span>

#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/serialization/EntityMapper.hpp"

/// @brief Writes the value of any replicated type into bytes, and reads it back, field by field from its TypeInfo.
///
/// @details Every field of `info.fields` is written in that order, and for each field its `count` values in order,
/// each in a fixed form whatever the machine: integers and floats little-endian, `bool` as a `u8` 0 or 1, vectors as
/// 2, 3 or 4 `f32`, entities as the `u32` NetworkId given by an EntityMapper, asset ids as a `u64`. Padding bytes are
/// never written. Nothing allocates or throws.
namespace rtype::replication::serialization {
/// @brief Tells whether values of a type can be sent: it has at least one field, and its values are plain data that
/// can be copied, moved and destroyed as bytes (no destruct, move or copy hook).
///
/// @details A construct hook is allowed: it only applies default member initializers, and readFields() writes every
/// byte of the value anyway. The manifest refuses to replicate a type that is not serializable.
/// @param info The description of the type.
/// @return True when writeFields() and readFields() can handle the type.
[[nodiscard]] RTYPE_REPLICATION_API bool isSerializable(const ecs::TypeInfo& info) noexcept;

/// @brief Gives the number of bytes writeFields() writes for a value of a type; the same for every value, since no
/// field has a variable size.
/// @param info The description of the type.
/// @return The size of one value on the wire.
[[nodiscard]] RTYPE_REPLICATION_API std::size_t computeWireSize(const ecs::TypeInfo& info) noexcept;

/// @brief Writes every field of a value; a value that does not fit makes the writer invalid.
/// @param writer Where to write.
/// @param info The description of the type, valid (see ecs::validateType()) and serializable.
/// @param value The bytes of one value of the type, at least `info.size` of them. Read with copies, so they need no
/// alignment.
/// @param mapper Gives the NetworkId of each entity field: 0 for a null or untracked entity.
RTYPE_REPLICATION_API void writeFields(network::bytes::ByteWriter& writer, const ecs::TypeInfo& info,
                                       std::span<const std::byte> value, const EntityMapper& mapper) noexcept;

/// @brief Zeroes a value, then reads every field of it.
/// @param reader Where to read, usually bytes received from the network.
/// @param info The description of the type, valid (see ecs::validateType()) and serializable.
/// @param value Where to build the value: zeroed, then filled; padding bytes stay zero.
/// @param mapper Gives the local entity of each entity field: the null entity for 0 or an unknown NetworkId.
/// @return False when bytes are missing, a `bool` is not 0 or 1, a float or a vector component is NaN or infinite,
/// or `value` is smaller than `info.size`. The content of `value` is then unspecified and must be thrown away.
[[nodiscard]] RTYPE_REPLICATION_API bool readFields(network::bytes::ByteReader& reader, const ecs::TypeInfo& info,
                                                    std::span<std::byte> value, const EntityMapper& mapper) noexcept;
}  // namespace rtype::replication::serialization
