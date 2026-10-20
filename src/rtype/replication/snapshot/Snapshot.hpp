/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Snapshot
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::replication::manifest {
class Manifest;
}  // namespace rtype::replication::manifest

namespace rtype::replication::snapshot {
/// @brief The prefab of a record that has none (always, until prefabs exist).
inline constexpr std::uint16_t kNoPrefab = 0xFFFF;

/// @brief Flag bit: the snapshot lists every replicated entity, with all its replicated components, so the client
/// removes what is absent. The other bits are reserved and must be 0.
inline constexpr std::uint8_t kComplete = 0x01;

/// @brief The most components one record holds: their count is a `u8`.
inline constexpr std::size_t kMaxRecordComponents = 255;

/// @brief The size of the snapshot's header: type (1), tick (4), last processed input (4) and flags (1).
inline constexpr std::size_t kHeaderSize = 10;

/// @brief Where the last processed input is in a message, to change it for each client.
inline constexpr std::size_t kLastProcessedInputOffset = 5;

/// @brief The size of a snapshot with no record: the header and the four counts (`u16`).
inline constexpr std::size_t kEmptySize = kHeaderSize + (4 * sizeof(std::uint16_t));

/// @brief The value of one component in a record: its fields, written by the field serializer.
struct ComponentValue {
    std::uint16_t shortId{0};  ///< The component's short id in the manifest.
    network::Bytes bytes;      ///< The fields, exactly the wire size of the component.

    friend bool operator==(const ComponentValue& left, const ComponentValue& right) noexcept = default;
};

/// @brief An entity the client does not know yet, with all its components.
struct SpawnRecord {
    std::uint32_t networkId{0};              ///< The entity's NetworkId, never 0.
    std::uint16_t prefab{kNoPrefab};         ///< The prefab, kNoPrefab for none.
    std::vector<ComponentValue> components;  ///< Its components, in increasing short id.

    friend bool operator==(const SpawnRecord& left, const SpawnRecord& right) noexcept = default;
};

/// @brief New values of components of an entity the client knows.
struct UpdateRecord {
    std::uint32_t networkId{0};              ///< The entity's NetworkId, never 0.
    std::vector<ComponentValue> components;  ///< The components that changed, in increasing short id.

    friend bool operator==(const UpdateRecord& left, const UpdateRecord& right) noexcept = default;
};

/// @brief A component the entity lost.
struct RemovalRecord {
    std::uint32_t networkId{0};  ///< The entity's NetworkId, never 0.
    std::uint16_t shortId{0};    ///< The component's short id.

    friend bool operator==(const RemovalRecord& left, const RemovalRecord& right) noexcept = default;
};

/// @brief The state of the world as the server sends it (kSnapshot), decoded: a header and four lists.
///
/// @details The component values stay bytes, per short id: turning them into components needs the manifest, which only
/// the plugins hold. Shared by both plugins: the server fills one and encodes it, the client decodes one.
struct Snapshot {
    std::uint32_t tick{0};                ///< The server tick of the snapshot.
    std::uint32_t lastProcessedInput{0};  ///< The last input of the receiving client the server applied, 0 if none.
    std::uint8_t flags{0};                ///< kComplete, or 0.
    std::vector<SpawnRecord> spawns;      ///< Entities to create, in increasing NetworkId.
    std::vector<UpdateRecord> updates;    ///< Entities to update.
    std::vector<RemovalRecord> removals;  ///< Components to remove.
    std::vector<std::uint32_t> despawns;  ///< NetworkIds of the entities to remove.

    friend bool operator==(const Snapshot& left, const Snapshot& right) noexcept = default;
};

/// @param record A record.
/// @return The bytes `record` takes in a snapshot.
[[nodiscard]] RTYPE_REPLICATION_API std::size_t computeWireSize(const SpawnRecord& record) noexcept;

/// @param snapshot A snapshot.
/// @return The bytes `snapshot` takes once encoded.
[[nodiscard]] RTYPE_REPLICATION_API std::size_t computeWireSize(const Snapshot& snapshot) noexcept;

/// @brief Writes a snapshot.
/// @param snapshot The snapshot to write.
/// @return The bytes, starting with the message type, or nothing when a list holds more than 65 535 records or a
/// record more than kMaxRecordComponents components.
[[nodiscard]] RTYPE_REPLICATION_API std::optional<network::Bytes> encode(const Snapshot& snapshot);

/// @brief Reads a snapshot from bytes of the network; never throws.
/// @param message The whole message, starting with the message type.
/// @param componentSizes The wire size of every component by short id (see getComponentSizes()); its length is the
/// number of components in the manifest.
/// @return The snapshot, or nothing when a count is larger than the remaining bytes allow, a short id is unknown or
/// listed twice in a record, a NetworkId is 0 or listed twice in a list, a reserved flag is set, or bytes are missing
/// or left over.
[[nodiscard]] RTYPE_REPLICATION_API std::optional<Snapshot> decode(
    std::span<const std::byte> message, std::span<const std::size_t> componentSizes) noexcept;

/// @brief Changes the last processed input of an encoded snapshot: the rest is the same for every client.
/// @param message A message made by encode(); a shorter one is left as it is.
/// @param sequence The new value.
RTYPE_REPLICATION_API void setLastProcessedInput(std::span<std::byte> message, std::uint32_t sequence) noexcept;

/// @brief Gives the size of every component's value on the wire, by short id.
/// @param manifest The manifest of the world.
/// @param world The world, whose components the manifest lists.
/// @return The sizes, one per component of the manifest.
[[nodiscard]] RTYPE_REPLICATION_API std::vector<std::size_t> computeComponentSizes(const manifest::Manifest& manifest,
                                                                               const ecs::World& world);
}  // namespace rtype::replication::snapshot
