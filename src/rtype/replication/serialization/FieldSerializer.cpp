/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldSerializer
*/

#include "rtype/replication/serialization/FieldSerializer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "core/hash/NameHash.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/serialization/EntityMapper.hpp"

namespace rtype::replication::serialization {
namespace {
/// @return The number of bytes one value of `type` takes on the wire.
constexpr std::size_t getValueWireSize(const ecs::FieldType type) noexcept {
    switch (type) {
        case ecs::FieldType::kBool:
        case ecs::FieldType::kI8:
        case ecs::FieldType::kU8:
        case ecs::FieldType::kText:
            return 1;
        case ecs::FieldType::kI16:
        case ecs::FieldType::kU16:
            return 2;
        case ecs::FieldType::kI32:
        case ecs::FieldType::kU32:
        case ecs::FieldType::kF32:
        case ecs::FieldType::kEntity:
            return 4;
        case ecs::FieldType::kI64:
        case ecs::FieldType::kU64:
        case ecs::FieldType::kF64:
        case ecs::FieldType::kAssetId:
        case ecs::FieldType::kVec2:
            return 8;
        case ecs::FieldType::kVec3:
            return 12;
        case ecs::FieldType::kVec4:
            return 16;
        case ecs::FieldType::kString:
            return 1 + ecs::String::kCapacity;  // The most: a length and the bytes.
    }
    return 0;  // Unreachable: every FieldType is handled above.
}

/// @return The number of floats of a vector type, 0 for any other type.
constexpr std::size_t getVectorLength(const ecs::FieldType type) noexcept {
    switch (type) {
        case ecs::FieldType::kVec2:
            return 2;
        case ecs::FieldType::kVec3:
            return 3;
        case ecs::FieldType::kVec4:
            return 4;
        default:
            return 0;
    }
}

/// @return A copy of the `T` stored at the start of `bytes`, which needs no alignment.
template <typename T>
T load(const std::span<const std::byte> bytes) noexcept {
    T result{};
    std::memcpy(&result, bytes.data(), sizeof(T));
    return result;
}

/// @brief Copies `value` to the start of `bytes`, which needs no alignment.
template <typename T>
void store(std::span<std::byte> bytes, const T& value) noexcept {
    std::memcpy(bytes.data(), &value, sizeof(T));
}

/// @brief Writes one value of `type`, read from the start of `bytes`.
void writeValue(network::bytes::ByteWriter& writer, ecs::FieldType type, std::span<const std::byte> bytes,
                const EntityMapper& mapper) noexcept {
    switch (type) {
        case ecs::FieldType::kBool:
            writer.writeU8(load<std::uint8_t>(bytes) != 0 ? 1 : 0);
            break;
        case ecs::FieldType::kI8:
            writer.writeI8(load<std::int8_t>(bytes));
            break;
        case ecs::FieldType::kI16:
            writer.writeI16(load<std::int16_t>(bytes));
            break;
        case ecs::FieldType::kI32:
            writer.writeI32(load<std::int32_t>(bytes));
            break;
        case ecs::FieldType::kI64:
            writer.writeI64(load<std::int64_t>(bytes));
            break;
        case ecs::FieldType::kU8:
        case ecs::FieldType::kText:
            writer.writeU8(load<std::uint8_t>(bytes));
            break;
        case ecs::FieldType::kU16:
            writer.writeU16(load<std::uint16_t>(bytes));
            break;
        case ecs::FieldType::kU32:
            writer.writeU32(load<std::uint32_t>(bytes));
            break;
        case ecs::FieldType::kU64:
        case ecs::FieldType::kAssetId:
            writer.writeU64(load<std::uint64_t>(bytes));
            break;
        case ecs::FieldType::kF32:
            writer.writeF32(load<float>(bytes));
            break;
        case ecs::FieldType::kF64:
            writer.writeF64(load<double>(bytes));
            break;
        case ecs::FieldType::kVec2:
        case ecs::FieldType::kVec3:
        case ecs::FieldType::kVec4:
            for (std::size_t i = 0; i < getVectorLength(type); ++i) {
                writer.writeF32(load<float>(bytes.subspan(i * sizeof(float))));
            }
            break;
        case ecs::FieldType::kEntity: {
            const auto entity = load<ecs::Entity>(bytes);
            writer.writeU32(entity.isNull() ? 0 : mapper.toNetwork(entity));
            break;
        }
        case ecs::FieldType::kString: {
            // The first byte of an ecs::String is its length, the text follows.
            const std::size_t length = std::min<std::size_t>(load<std::uint8_t>(bytes), ecs::String::kCapacity);
            writer.writeU8(static_cast<std::uint8_t>(length));
            writer.writeBytes(bytes.subspan(1, length));
            break;
        }
    }
}

/// @brief Reads a float and stores it at the start of `bytes`.
/// @return False when it is NaN or infinite.
bool readFloat(network::bytes::ByteReader& reader, std::span<std::byte> bytes) noexcept {
    const float value = reader.readF32();
    store(bytes, value);
    return std::isfinite(value);
}

/// @brief Reads one value of `type` and stores it at the start of `bytes`.
/// @return False when the value is invalid: a `bool` other than 0 or 1, a NaN or infinite float.
bool readValue(network::bytes::ByteReader& reader, ecs::FieldType type, std::span<std::byte> bytes,
               const EntityMapper& mapper) noexcept {
    switch (type) {
        case ecs::FieldType::kBool: {
            const std::uint8_t value = reader.readU8();
            store(bytes, value);
            return value <= 1;
        }
        case ecs::FieldType::kI8:
            store(bytes, reader.readI8());
            return true;
        case ecs::FieldType::kI16:
            store(bytes, reader.readI16());
            return true;
        case ecs::FieldType::kI32:
            store(bytes, reader.readI32());
            return true;
        case ecs::FieldType::kI64:
            store(bytes, reader.readI64());
            return true;
        case ecs::FieldType::kU8:
        case ecs::FieldType::kText:
            store(bytes, reader.readU8());
            return true;
        case ecs::FieldType::kU16:
            store(bytes, reader.readU16());
            return true;
        case ecs::FieldType::kU32:
            store(bytes, reader.readU32());
            return true;
        case ecs::FieldType::kU64:
        case ecs::FieldType::kAssetId:
            store(bytes, reader.readU64());
            return true;
        case ecs::FieldType::kF32:
            return readFloat(reader, bytes);
        case ecs::FieldType::kF64: {
            const double value = reader.readF64();
            store(bytes, value);
            return std::isfinite(value);
        }
        case ecs::FieldType::kVec2:
        case ecs::FieldType::kVec3:
        case ecs::FieldType::kVec4:
            for (std::size_t i = 0; i < getVectorLength(type); ++i) {
                if (!readFloat(reader, bytes.subspan(i * sizeof(float)))) {
                    return false;
                }
            }
            return true;
        case ecs::FieldType::kEntity: {
            const std::uint32_t networkId = reader.readU32();
            store(bytes, networkId == 0 ? ecs::Entity{} : mapper.toLocal(networkId));
            return true;
        }
        case ecs::FieldType::kString: {
            const std::uint8_t length = reader.readU8();
            const std::span<const std::byte> text = reader.readBytes(length);
            if (!reader.isValid()) {
                return false;
            }
            bytes.front() = static_cast<std::byte>(length);
            std::ranges::copy(text, bytes.begin() + 1);
            return true;
        }
    }
    return false;  // Unreachable: every FieldType is handled above.
}

/// @return Whether a field is a list: a count and then its values, on the wire.
bool isList(const ecs::FieldInfo& field) noexcept { return field.list != nullptr; }

/// @return The values of a list field of `value`, a live value of its type.
std::span<const std::byte> listOf(const ecs::FieldInfo& field, std::span<const std::byte> value) noexcept {
    return field.list->elements(value.subspan(field.offset).data());
}

/// @return How many values `values`, the bytes of a list field, holds.
std::size_t countValues(const ecs::FieldInfo& field, std::span<const std::byte> values) noexcept {
    return values.size() / ecs::getFieldLayout(field.type).size;
}

/// @brief Reads a list field into `value`, a live value of its type. An entity that is null or unknown here is left
/// out: a list has no use for a null entity.
/// @return False when bytes are missing, a value is invalid, or the count announces more values than the bytes left can
/// hold: it comes from the network, so it is checked before anything is allocated.
bool readList(network::bytes::ByteReader& reader, const ecs::FieldInfo& field, std::span<std::byte> value,
              const EntityMapper& mapper) noexcept {
    const std::size_t count = reader.readU16();
    const std::size_t smallest = field.type == ecs::FieldType::kString ? 1 : getValueWireSize(field.type);
    if (!reader.isValid() || count > reader.remainingSize() / smallest) {
        return false;
    }
    const std::size_t valueSize = ecs::getFieldLayout(field.type).size;
    try {
        std::vector<std::byte> values(count * valueSize);
        std::size_t kept = 0;
        for (std::size_t i = 0; i < count; ++i) {
            const std::span<std::byte> slot = std::span{values}.subspan(kept * valueSize, valueSize);
            if (!readValue(reader, field.type, slot, mapper)) {
                return false;
            }
            if (field.type != ecs::FieldType::kEntity || !load<ecs::Entity>(slot).isNull()) {
                ++kept;
            }
        }
        field.list->assign(value.subspan(field.offset).data(), std::span{values}.first(kept * valueSize));
    } catch (...) {  // out of memory: a value that cannot be built is a value that was not received
        return false;
    }
    return true;
}
}  // namespace

bool isSerializable(const ecs::TypeInfo& info) noexcept {
    if (info.fields.empty()) {
        return false;
    }
    const bool plain = info.hooks.destruct == nullptr && info.hooks.move == nullptr && info.hooks.copy == nullptr;
    const bool hasList = std::ranges::any_of(info.fields, &isList);
    return plain || (hasList && info.copyable && info.hooks.construct != nullptr);
}

bool isVariableSize(const ecs::TypeInfo& info) noexcept {
    return std::ranges::any_of(info.fields, [](const ecs::FieldInfo& field) {
        return field.type == ecs::FieldType::kString || isList(field);
    });
}

std::uint64_t computeManifestHash(const ecs::TypeInfo& info) noexcept {
    if (std::ranges::none_of(info.fields, &isList)) {
        return info.layoutHash;
    }
    std::uint64_t hash = core::hash::kNameHashOffsetBasis;
    hash = core::hash::hashString(hash, info.name);
    for (const ecs::FieldInfo& field : info.fields) {
        hash = core::hash::hashInteger(hash, field.name.size());
        hash = core::hash::hashString(hash, field.name);
        hash = core::hash::hashInteger(hash, static_cast<std::uint64_t>(field.type));
        hash = core::hash::hashInteger(hash, field.count);
        hash = core::hash::hashInteger(hash, isList(field) ? 1U : 0U);
    }
    return hash;
}

std::size_t computeWireSize(const ecs::TypeInfo& info) noexcept {
    std::size_t size = 0;
    for (const ecs::FieldInfo& field : info.fields) {
        size += isList(field) ? sizeof(std::uint16_t) : getValueWireSize(field.type) * field.count;
    }
    return size;
}

std::size_t computeWireSize(const ecs::TypeInfo& info, std::span<const std::byte> value) noexcept {
    std::size_t size = computeWireSize(info);
    for (const ecs::FieldInfo& field : info.fields) {
        if (isList(field)) {
            size += countValues(field, listOf(field, value)) * getValueWireSize(field.type);
        }
    }
    return size;
}

void writeFields(network::bytes::ByteWriter& writer, const ecs::TypeInfo& info, std::span<const std::byte> value,
                 const EntityMapper& mapper) noexcept {
    for (const ecs::FieldInfo& field : info.fields) {
        const std::size_t valueSize = ecs::getFieldLayout(field.type).size;
        if (isList(field)) {
            const std::span<const std::byte> values = listOf(field, value);
            const std::size_t count = countValues(field, values);
            writer.writeU16(static_cast<std::uint16_t>(count));
            for (std::size_t i = 0; i < count; ++i) {
                writeValue(writer, field.type, values.subspan(i * valueSize, valueSize), mapper);
            }
            continue;
        }
        for (std::size_t i = 0; i < field.count; ++i) {
            writeValue(writer, field.type, value.subspan(field.offset + (i * valueSize), valueSize), mapper);
        }
    }
}

bool readFields(network::bytes::ByteReader& reader, const ecs::TypeInfo& info, std::span<std::byte> value,
                const EntityMapper& mapper) noexcept {
    if (value.size() < info.size) {
        return false;
    }
    if (info.hooks.destruct == nullptr) {
        std::ranges::fill(value, std::byte{0});  // a value that owns memory is a live object: its bytes are not ours
    }
    for (const ecs::FieldInfo& field : info.fields) {
        const std::size_t valueSize = ecs::getFieldLayout(field.type).size;
        if (isList(field)) {
            if (!readList(reader, field, value, mapper)) {
                return false;
            }
            continue;
        }
        for (std::size_t i = 0; i < field.count; ++i) {
            if (!readValue(reader, field.type, value.subspan(field.offset + (i * valueSize), valueSize), mapper)) {
                return false;
            }
        }
    }
    return reader.isValid();
}
}  // namespace rtype::replication::serialization
