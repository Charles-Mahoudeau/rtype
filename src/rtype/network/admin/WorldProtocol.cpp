/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldProtocol
*/

#include "rtype/network/admin/WorldProtocol.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"

namespace rtype::network::admin {
namespace {
constexpr std::size_t kWriteCapacity = kMaxWorldBody + 256;  // room to notice an overflow
constexpr std::size_t kEntityEntryMin = 8 + 1 + 2;           // handle, empty name, component count
constexpr std::size_t kComponentMin = 1 + 1;                 // empty name, field count
constexpr std::size_t kFieldMin = 1 + 1 + 2 + 1 + 1;         // name, type, count, value, editable

constexpr std::uint8_t toByte(const WorldMessage type) noexcept { return static_cast<std::uint8_t>(type); }

/// @return @p text cut to @p limit bytes, with "..." in place of the end when @p marked.
std::string cut(const std::string_view text, const std::size_t limit, const bool marked = false) {
    if (text.size() <= limit) {
        return std::string{text};
    }
    if (!marked || limit < 3) {
        return std::string{text.substr(0, limit)};
    }
    return std::string{text.substr(0, limit - 3)} + "...";
}

std::size_t stringSize(const std::string_view text) noexcept { return 1 + text.size(); }

void writeFieldView(bytes::ByteWriter& writer, const FieldView& field) {
    writer.writeString(cut(field.name, kMaxWorldName));
    writer.writeString(cut(field.type, kMaxWorldName));
    writer.writeU16(field.count);
    writer.writeString(cut(field.value, kMaxWorldValue, true));
    writer.writeU8(field.editable ? 1 : 0);
}

void writeComponentView(bytes::ByteWriter& writer, const ComponentView& component) {
    writer.writeString(cut(component.name, kMaxWorldName));
    writer.writeU8(static_cast<std::uint8_t>(std::min<std::size_t>(component.fields.size(), 255)));
    for (std::size_t i = 0; i < component.fields.size() && i < 255; ++i) {
        writeFieldView(writer, component.fields.at(i));
    }
}

void writeEntityEntry(bytes::ByteWriter& writer, const EntityEntry& entry) {
    writer.writeU64(entry.entity);
    writer.writeString(cut(entry.name, kMaxWorldName));
    writer.writeU16(entry.componentCount);
}

FieldView readFieldView(bytes::ByteReader& reader) {
    FieldView field;
    field.name = reader.readString();
    field.type = reader.readString();
    field.count = reader.readU16();
    field.value = reader.readString();
    field.editable = reader.readU8() != 0;
    return field;
}

ComponentView readComponentView(bytes::ByteReader& reader) {
    ComponentView component;
    component.name = reader.readString();
    const std::size_t count = reader.readU8();
    if (!reader.isValid() || count * kFieldMin > reader.remainingSize()) {
        component.name.clear();
        static_cast<void>(reader.readBytes(reader.remainingSize() + 1));  // makes the reader invalid
        return component;
    }
    component.fields.reserve(count);
    for (std::size_t i = 0; i < count && reader.isValid(); ++i) {
        component.fields.push_back(readFieldView(reader));
    }
    return component;
}

/// @brief Poisons @p reader when @p count items of at least @p minimum bytes cannot fit in what is left.
bool fits(bytes::ByteReader& reader, const std::size_t count, const std::size_t minimum) {
    if (!reader.isValid() || count * minimum > reader.remainingSize()) {
        static_cast<void>(reader.readBytes(reader.remainingSize() + 1));
        return false;
    }
    return true;
}

bool isWorldType(const std::uint8_t type) noexcept {
    return type >= toByte(WorldMessage::kEntities) && type <= toByte(WorldMessage::kRun);
}
}  // namespace

std::size_t encodedSize(const EntityEntry& entry) noexcept {
    return 8 + stringSize(cut(entry.name, kMaxWorldName)) + 2;
}

std::size_t encodedSize(const ComponentView& component) noexcept {
    std::size_t size = stringSize(cut(component.name, kMaxWorldName)) + 1;
    for (std::size_t i = 0; i < component.fields.size() && i < 255; ++i) {
        const FieldView& field = component.fields.at(i);
        size += stringSize(cut(field.name, kMaxWorldName)) + stringSize(cut(field.type, kMaxWorldName)) + 2 +
                stringSize(cut(field.value, kMaxWorldValue, true)) + 1;
    }
    return size;
}

std::size_t entitiesPrefixSize() noexcept { return 4 + 4 + 2; }

std::size_t inspectPrefixSize(const std::size_t nameSize) noexcept {
    return 8 + stringSize(std::string(std::min(nameSize, kMaxWorldName), 'x')) + 2 + 2 + 1;
}

ExtensionRequest encodeWorldRequest(const WorldRequest& request) {
    Bytes buffer(kWriteCapacity);
    bytes::ByteWriter writer{buffer};
    ExtensionRequest result;
    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, EntitiesRequest>) {
                result.type = toByte(WorldMessage::kEntities);
                writer.writeU32(value.first);
            } else if constexpr (std::is_same_v<T, InspectRequest>) {
                result.type = toByte(WorldMessage::kInspect);
                writer.writeU64(value.entity);
                writer.writeU16(value.firstComponent);
            } else if constexpr (std::is_same_v<T, SetFieldRequest>) {
                result.type = toByte(WorldMessage::kSetField);
                writer.writeU64(value.entity);
                writer.writeString(cut(value.component, kMaxWorldName));
                writer.writeString(cut(value.field, kMaxWorldName));
                writer.writeU16(value.index);
                writer.writeString(cut(value.value, kMaxWorldValue));
            } else if constexpr (std::is_same_v<T, SpawnRequest>) {
                result.type = toByte(WorldMessage::kSpawn);
                writer.writeString(cut(value.name, kMaxWorldName));
            } else if constexpr (std::is_same_v<T, DespawnRequest>) {
                result.type = toByte(WorldMessage::kDespawn);
                writer.writeU64(value.entity);
            } else {
                result.type = toByte(WorldMessage::kRun);
                writer.writeU8(static_cast<std::uint8_t>(value.action));
            }
        },
        request);
    const auto written = writer.writtenBytes();
    result.body.assign(written.begin(), written.end());
    return result;
}

std::optional<WorldRequest> decodeWorldRequest(const ExtensionRequest& request) {
    bytes::ByteReader reader{request.body};
    std::optional<WorldRequest> result;
    switch (request.type) {
        case toByte(WorldMessage::kEntities):
            result = EntitiesRequest{.first = reader.readU32()};
            break;
        case toByte(WorldMessage::kInspect): {
            InspectRequest inspect;
            inspect.entity = reader.readU64();
            inspect.firstComponent = reader.readU16();
            result = inspect;
            break;
        }
        case toByte(WorldMessage::kSetField): {
            SetFieldRequest set;
            set.entity = reader.readU64();
            set.component = reader.readString();
            set.field = reader.readString();
            set.index = reader.readU16();
            set.value = reader.readString();
            result = std::move(set);
            break;
        }
        case toByte(WorldMessage::kSpawn):
            result = SpawnRequest{.name = reader.readString()};
            break;
        case toByte(WorldMessage::kDespawn):
            result = DespawnRequest{.entity = reader.readU64()};
            break;
        case toByte(WorldMessage::kRun): {
            const std::uint8_t action = reader.readU8();
            if (action > static_cast<std::uint8_t>(RunAction::kStep)) {
                return std::nullopt;
            }
            result = RunRequest{.action = static_cast<RunAction>(action)};
            break;
        }
        default:
            return std::nullopt;
    }
    if (!reader.isValid() || reader.remainingSize() != 0) {
        return std::nullopt;
    }
    return result;
}

ExtensionReply encodeWorldReply(const WorldReply& reply) {
    Bytes buffer(kWriteCapacity);
    bytes::ByteWriter writer{buffer};
    ExtensionReply result;
    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, EntitiesReply>) {
                result.type = toByte(WorldMessage::kEntities);
                writer.writeU32(value.total);
                writer.writeU32(value.first);
                writer.writeU16(static_cast<std::uint16_t>(std::min<std::size_t>(value.entries.size(), 0xFFFF)));
                for (const EntityEntry& entry : value.entries) {
                    writeEntityEntry(writer, entry);
                }
            } else if constexpr (std::is_same_v<T, InspectReply>) {
                result.type = toByte(WorldMessage::kInspect);
                writer.writeU64(value.entity);
                writer.writeString(cut(value.name, kMaxWorldName));
                writer.writeU16(value.total);
                writer.writeU16(value.next);
                writer.writeU8(static_cast<std::uint8_t>(std::min<std::size_t>(value.components.size(), 255)));
                for (std::size_t i = 0; i < value.components.size() && i < 255; ++i) {
                    writeComponentView(writer, value.components.at(i));
                }
            } else if constexpr (std::is_same_v<T, SetFieldReply>) {
                result.type = toByte(WorldMessage::kSetField);
            } else if constexpr (std::is_same_v<T, SpawnReply>) {
                result.type = toByte(WorldMessage::kSpawn);
                writer.writeU64(value.entity);
            } else if constexpr (std::is_same_v<T, DespawnReply>) {
                result.type = toByte(WorldMessage::kDespawn);
                writer.writeU64(value.entity);
            } else {
                result.type = toByte(WorldMessage::kRun);
                writer.writeU8(value.paused ? 1 : 0);
            }
        },
        reply);
    result.type = static_cast<std::uint8_t>(result.type | kReplyFlag);
    if (!writer.isValid() || writer.bytesWritten() > kMaxWorldBody) {
        return result;  // too long: an empty body, which decodes to nothing
    }
    const auto written = writer.writtenBytes();
    result.body.assign(written.begin(), written.end());
    return result;
}

std::optional<WorldReply> decodeWorldReply(const ExtensionReply& reply) {
    if ((reply.type & kReplyFlag) == 0 || !isWorldType(static_cast<std::uint8_t>(reply.type & 0x7FU))) {
        return std::nullopt;
    }
    bytes::ByteReader reader{reply.body};
    std::optional<WorldReply> result;
    switch (static_cast<WorldMessage>(reply.type & 0x7FU)) {
        case WorldMessage::kEntities: {
            EntitiesReply entities;
            entities.total = reader.readU32();
            entities.first = reader.readU32();
            const std::size_t count = reader.readU16();
            if (fits(reader, count, kEntityEntryMin)) {
                entities.entries.reserve(count);
                for (std::size_t i = 0; i < count && reader.isValid(); ++i) {
                    EntityEntry entry;
                    entry.entity = reader.readU64();
                    entry.name = reader.readString();
                    entry.componentCount = reader.readU16();
                    entities.entries.push_back(std::move(entry));
                }
            }
            result = std::move(entities);
            break;
        }
        case WorldMessage::kInspect: {
            InspectReply inspect;
            inspect.entity = reader.readU64();
            inspect.name = reader.readString();
            inspect.total = reader.readU16();
            inspect.next = reader.readU16();
            const std::size_t count = reader.readU8();
            if (fits(reader, count, kComponentMin)) {
                inspect.components.reserve(count);
                for (std::size_t i = 0; i < count && reader.isValid(); ++i) {
                    inspect.components.push_back(readComponentView(reader));
                }
            }
            result = std::move(inspect);
            break;
        }
        case WorldMessage::kSetField:
            result = SetFieldReply{};
            break;
        case WorldMessage::kSpawn:
            result = SpawnReply{.entity = reader.readU64()};
            break;
        case WorldMessage::kDespawn:
            result = DespawnReply{.entity = reader.readU64()};
            break;
        case WorldMessage::kRun:
            result = RunReply{.paused = reader.readU8() != 0};
            break;
    }
    if (!reader.isValid() || reader.remainingSize() != 0) {
        return std::nullopt;
    }
    return result;
}

}  // namespace rtype::network::admin
