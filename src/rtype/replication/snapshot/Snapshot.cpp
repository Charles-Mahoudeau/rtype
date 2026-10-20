/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Snapshot
*/

#include "rtype/replication/snapshot/Snapshot.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

#include "rtype/ecs/world/World.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/serialization/FieldSerializer.hpp"

namespace rtype::replication::snapshot {
namespace {
/// @brief The most records a list holds: a count is a `u16`.
constexpr std::size_t kMaxRecords = std::numeric_limits<std::uint16_t>::max();

/// @brief NetworkId (4), prefab (2) and component count (1).
constexpr std::size_t kSpawnFixedSize = 7;

/// @brief NetworkId (4) and component count (1).
constexpr std::size_t kUpdateFixedSize = 5;

/// @brief NetworkId (4) and short id (2).
constexpr std::size_t kRemovalSize = 6;

/// @brief NetworkId (4).
constexpr std::size_t kDespawnSize = 4;

/// @brief Short id (2): what a component takes besides its fields.
constexpr std::size_t kComponentFixedSize = 2;

[[nodiscard]] std::size_t getComponentsSize(const std::vector<ComponentValue>& components) noexcept {
    return std::accumulate(components.begin(), components.end(), std::size_t{0},
                           [](const std::size_t total, const ComponentValue& component) {
                               return total + kComponentFixedSize + component.bytes.size();
                           });
}

[[nodiscard]] std::size_t computeWireSize(const UpdateRecord& record) noexcept {
    return kUpdateFixedSize + getComponentsSize(record.components);
}

void writeComponents(network::bytes::ByteWriter& writer, const std::vector<ComponentValue>& components) noexcept {
    writer.writeU8(static_cast<std::uint8_t>(components.size()));
    for (const ComponentValue& component : components) {
        writer.writeU16(component.shortId);
        writer.writeBytes(component.bytes);
    }
}

/// @return True when no list or record is larger than its count allows.
[[nodiscard]] bool fitsCounts(const Snapshot& snapshot) noexcept {
    const auto tooMany = [](const auto& records) {
        return std::ranges::any_of(records,
                                   [](const auto& record) { return record.components.size() > kMaxRecordComponents; });
    };
    return snapshot.spawns.size() <= kMaxRecords && snapshot.updates.size() <= kMaxRecords &&
           snapshot.removals.size() <= kMaxRecords && snapshot.despawns.size() <= kMaxRecords &&
           !tooMany(snapshot.spawns) && !tooMany(snapshot.updates);
}

/// @brief Reads the components of one record.
/// @return False when a short id is unknown or listed twice, or bytes are missing.
[[nodiscard]] bool readComponents(network::bytes::ByteReader& reader, const std::span<const std::size_t> sizes,
                                  std::vector<ComponentValue>& components) {
    const std::size_t count = reader.readU8();
    components.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const std::uint16_t shortId = reader.readU16();
        if (!reader.isValid() || shortId >= sizes.size() ||
            std::ranges::any_of(components,
                                [shortId](const ComponentValue& known) { return known.shortId == shortId; })) {
            return false;
        }
        const std::span<const std::byte> value = reader.readBytes(sizes[shortId]);
        if (!reader.isValid()) {
            return false;
        }
        components.push_back({.shortId = shortId, .bytes = network::Bytes{value.begin(), value.end()}});
    }
    return reader.isValid();
}

/// @brief Reads a list count, refusing one that the remaining bytes cannot hold at `minimumSize` each.
[[nodiscard]] std::optional<std::size_t> readCount(network::bytes::ByteReader& reader,
                                                   const std::size_t minimumSize) noexcept {
    const std::size_t count = reader.readU16();
    if (!reader.isValid() || count > reader.remainingSize() / minimumSize) {
        return std::nullopt;
    }
    return count;
}

/// @brief Tells whether a NetworkId is valid and new to the list, and remembers it.
[[nodiscard]] bool takeId(std::unordered_set<std::uint32_t>& seen, const std::uint32_t networkId) {
    return networkId != 0 && seen.insert(networkId).second;
}

[[nodiscard]] bool readSpawns(network::bytes::ByteReader& reader, const std::span<const std::size_t> sizes,
                              std::vector<SpawnRecord>& spawns) {
    const std::optional<std::size_t> count = readCount(reader, kSpawnFixedSize);
    if (!count) {
        return false;
    }
    std::unordered_set<std::uint32_t> seen;
    spawns.reserve(*count);
    for (std::size_t index = 0; index < *count; ++index) {
        SpawnRecord record;
        record.networkId = reader.readU32();
        record.prefab = reader.readU16();
        if (!reader.isValid() || !takeId(seen, record.networkId) || !readComponents(reader, sizes, record.components)) {
            return false;
        }
        spawns.push_back(std::move(record));
    }
    return true;
}

[[nodiscard]] bool readUpdates(network::bytes::ByteReader& reader, const std::span<const std::size_t> sizes,
                               std::vector<UpdateRecord>& updates) {
    const std::optional<std::size_t> count = readCount(reader, kUpdateFixedSize);
    if (!count) {
        return false;
    }
    std::unordered_set<std::uint32_t> seen;
    updates.reserve(*count);
    for (std::size_t index = 0; index < *count; ++index) {
        UpdateRecord record;
        record.networkId = reader.readU32();
        if (!reader.isValid() || !takeId(seen, record.networkId) || !readComponents(reader, sizes, record.components)) {
            return false;
        }
        updates.push_back(std::move(record));
    }
    return true;
}

[[nodiscard]] bool readRemovals(network::bytes::ByteReader& reader, const std::span<const std::size_t> sizes,
                                std::vector<RemovalRecord>& removals) {
    const std::optional<std::size_t> count = readCount(reader, kRemovalSize);
    if (!count) {
        return false;
    }
    removals.reserve(*count);
    for (std::size_t index = 0; index < *count; ++index) {
        const std::uint32_t networkId = reader.readU32();
        const std::uint16_t shortId = reader.readU16();
        if (!reader.isValid() || networkId == 0 || shortId >= sizes.size()) {
            return false;
        }
        removals.push_back({.networkId = networkId, .shortId = shortId});
    }
    return true;
}

[[nodiscard]] bool readDespawns(network::bytes::ByteReader& reader, std::vector<std::uint32_t>& despawns) {
    const std::optional<std::size_t> count = readCount(reader, kDespawnSize);
    if (!count) {
        return false;
    }
    std::unordered_set<std::uint32_t> seen;
    despawns.reserve(*count);
    for (std::size_t index = 0; index < *count; ++index) {
        const std::uint32_t networkId = reader.readU32();
        if (!reader.isValid() || !takeId(seen, networkId)) {
            return false;
        }
        despawns.push_back(networkId);
    }
    return true;
}
}  // namespace

std::size_t computeWireSize(const SpawnRecord& record) noexcept {
    return kSpawnFixedSize + getComponentsSize(record.components);
}

std::size_t computeWireSize(const Snapshot& snapshot) noexcept {
    std::size_t size = kEmptySize;
    for (const SpawnRecord& record : snapshot.spawns) {
        size += computeWireSize(record);
    }
    for (const UpdateRecord& record : snapshot.updates) {
        size += computeWireSize(record);
    }
    return size + snapshot.removals.size() * kRemovalSize + snapshot.despawns.size() * kDespawnSize;
}

std::optional<network::Bytes> encode(const Snapshot& snapshot) {
    if (!fitsCounts(snapshot)) {
        return std::nullopt;
    }
    network::Bytes bytes(computeWireSize(snapshot));
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kSnapshot));
    writer.writeU32(snapshot.tick);
    writer.writeU32(snapshot.lastProcessedInput);
    writer.writeU8(snapshot.flags);
    writer.writeU16(static_cast<std::uint16_t>(snapshot.spawns.size()));
    for (const SpawnRecord& record : snapshot.spawns) {
        writer.writeU32(record.networkId);
        writer.writeU16(record.prefab);
        writeComponents(writer, record.components);
    }
    writer.writeU16(static_cast<std::uint16_t>(snapshot.updates.size()));
    for (const UpdateRecord& record : snapshot.updates) {
        writer.writeU32(record.networkId);
        writeComponents(writer, record.components);
    }
    writer.writeU16(static_cast<std::uint16_t>(snapshot.removals.size()));
    for (const RemovalRecord& record : snapshot.removals) {
        writer.writeU32(record.networkId);
        writer.writeU16(record.shortId);
    }
    writer.writeU16(static_cast<std::uint16_t>(snapshot.despawns.size()));
    for (const std::uint32_t networkId : snapshot.despawns) {
        writer.writeU32(networkId);
    }
    if (!writer.isValid() || writer.bytesWritten() != bytes.size()) {
        return std::nullopt;
    }
    return bytes;
}

std::optional<Snapshot> decode(const std::span<const std::byte> message,
                               const std::span<const std::size_t> componentSizes) noexcept {
    try {
        network::bytes::ByteReader reader{message};
        if (reader.readU8() != static_cast<std::uint8_t>(MessageType::kSnapshot)) {
            return std::nullopt;
        }
        Snapshot snapshot;
        snapshot.tick = reader.readU32();
        snapshot.lastProcessedInput = reader.readU32();
        snapshot.flags = reader.readU8();
        if (!reader.isValid() || (snapshot.flags & ~kComplete) != 0 ||
            !readSpawns(reader, componentSizes, snapshot.spawns) ||
            !readUpdates(reader, componentSizes, snapshot.updates) ||
            !readRemovals(reader, componentSizes, snapshot.removals) || !readDespawns(reader, snapshot.despawns) ||
            !reader.isFinished()) {
            return std::nullopt;
        }
        return snapshot;
    } catch (...) {  // only an allocation can fail here
        return std::nullopt;
    }
}

void setLastProcessedInput(const std::span<std::byte> message, const std::uint32_t sequence) noexcept {
    if (message.size() < kLastProcessedInputOffset + sizeof(std::uint32_t)) {
        return;
    }
    network::bytes::ByteWriter writer{message.subspan(kLastProcessedInputOffset, sizeof(std::uint32_t))};
    writer.writeU32(sequence);
}

std::vector<std::size_t> computeComponentSizes(const manifest::Manifest& manifest, const ecs::World& world) {
    const auto& registry = world.getComponents().getRegistry();
    std::vector<std::size_t> sizes;
    sizes.reserve(manifest.components().size());
    for (const ecs::ComponentId id : manifest.components()) {
        const ecs::ComponentInfo* info = registry.findById(id);
        sizes.push_back(info == nullptr ? 0 : serialization::computeWireSize(info->type));
    }
    return sizes;
}
}  // namespace rtype::replication::snapshot
