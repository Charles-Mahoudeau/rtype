/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SnapshotTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace {
namespace snapshot = rtype::replication::snapshot;
using rtype::network::Bytes;
using snapshot::ComponentValue;
using snapshot::RemovalRecord;
using snapshot::Snapshot;
using snapshot::SpawnRecord;
using snapshot::UpdateRecord;

/// Component 0 takes 4 bytes on the wire, component 1 takes 1, component 2 takes 8.
constexpr std::array<std::size_t, 3> kSizes{4, 1, 8};

Bytes makeBytes(const std::size_t count, const std::uint8_t first) {
    Bytes bytes(count);
    for (std::size_t index = 0; index < count; ++index) {
        bytes[index] = static_cast<std::byte>(first + index);
    }
    return bytes;
}

Snapshot makeSnapshot() {
    Snapshot value;
    value.tick = 120;
    value.lastProcessedInput = 7;
    value.flags = snapshot::kComplete;
    value.spawns.push_back(
        {.networkId = 1,
         .prefab = snapshot::kNoPrefab,
         .components = {{.shortId = 0, .bytes = makeBytes(4, 1)}, {.shortId = 2, .bytes = makeBytes(8, 9)}}});
    value.spawns.push_back({.networkId = 5, .prefab = 3, .components = {{.shortId = 1, .bytes = makeBytes(1, 40)}}});
    value.updates.push_back({.networkId = 2, .components = {{.shortId = 1, .bytes = makeBytes(1, 50)}}});
    value.removals.push_back({.networkId = 3, .shortId = 2});
    value.despawns = {4, 6};
    return value;
}

std::span<const std::size_t> sizes() { return kSizes; }

/// @return The snapshot of makeSnapshot() encoded.
Bytes encodeSample() { return snapshot::encode(makeSnapshot()).value(); }
}  // namespace

TEST(Snapshot, EncodesTheDocumentedLayout) {
    Snapshot value;
    value.tick = 0x01020304;
    value.lastProcessedInput = 0x0A0B0C0D;
    value.flags = snapshot::kComplete;
    value.spawns.push_back(
        {.networkId = 9, .prefab = 0x0102, .components = {{.shortId = 1, .bytes = makeBytes(1, 7)}}});

    const std::optional<Bytes> bytes = snapshot::encode(value);

    ASSERT_TRUE(bytes);
    const Bytes expected{
        std::byte{0x30},                                                     // type
        std::byte{0x04}, std::byte{0x03}, std::byte{0x02}, std::byte{0x01},  // tick
        std::byte{0x0D}, std::byte{0x0C}, std::byte{0x0B}, std::byte{0x0A},  // last processed input
        std::byte{0x01},                                                     // flags
        std::byte{0x01}, std::byte{0x00},                                    // spawn count
        std::byte{0x09}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},  // net id
        std::byte{0x02}, std::byte{0x01},                                    // prefab
        std::byte{0x01},                                                     // component count
        std::byte{0x01}, std::byte{0x00}, std::byte{0x07},                   // short id, fields
        std::byte{0x00}, std::byte{0x00},                                    // update count
        std::byte{0x00}, std::byte{0x00},                                    // removal count
        std::byte{0x00}, std::byte{0x00},                                    // despawn count
    };
    EXPECT_EQ(*bytes, expected);
}

TEST(Snapshot, StartsWithTheSnapshotMessageType) {
    EXPECT_EQ(static_cast<std::uint8_t>(rtype::replication::MessageType::kSnapshot), 0x30);
    EXPECT_EQ(encodeSample().front(), std::byte{0x30});
}

TEST(Snapshot, WireSizeMatchesTheEncodedSize) {
    const Snapshot value = makeSnapshot();
    EXPECT_EQ(snapshot::computeWireSize(value), encodeSample().size());
    EXPECT_EQ(snapshot::computeWireSize(Snapshot{}), snapshot::kEmptySize);
    EXPECT_EQ(snapshot::computeWireSize(value.spawns.front()), 7U + (2 + 4) + (2 + 8));
}

TEST(Snapshot, RoundTrips) {
    const Snapshot value = makeSnapshot();
    const std::optional<Snapshot> decoded = snapshot::decode(encodeSample(), sizes());
    ASSERT_TRUE(decoded);
    EXPECT_EQ(*decoded, value);
}

TEST(Snapshot, EmptySnapshotRoundTrips) {
    const Bytes bytes = snapshot::encode(Snapshot{}).value();
    EXPECT_EQ(bytes.size(), snapshot::kEmptySize);
    EXPECT_EQ(snapshot::decode(bytes, sizes()), Snapshot{});
}

TEST(Snapshot, EncodeRefusesMoreRecordsThanACountHolds) {
    Snapshot value;
    value.despawns.assign(70000, 1U);
    EXPECT_FALSE(snapshot::encode(value));
}

TEST(Snapshot, EncodeRefusesMoreComponentsThanACountHolds) {
    Snapshot value;
    SpawnRecord record{.networkId = 1};
    record.components.assign(256, ComponentValue{.shortId = 0, .bytes = makeBytes(4, 0)});
    value.spawns.push_back(record);
    EXPECT_FALSE(snapshot::encode(value));
}

TEST(Snapshot, SetsTheLastProcessedInputOfAnEncodedMessage) {
    Bytes bytes = encodeSample();
    snapshot::setLastProcessedInput(bytes, 0xDEADBEEF);
    const std::optional<Snapshot> decoded = snapshot::decode(bytes, sizes());
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->lastProcessedInput, 0xDEADBEEF);
    EXPECT_EQ(decoded->tick, 120U);
    EXPECT_EQ(decoded->spawns.size(), 2U);
}

TEST(Snapshot, SetLastProcessedInputLeavesAShortMessageAlone) {
    Bytes bytes(6, std::byte{1});
    snapshot::setLastProcessedInput(bytes, 9);
    EXPECT_EQ(bytes, Bytes(6, std::byte{1}));
}

TEST(SnapshotDecoder, RejectsAnEmptyMessageAndTheWrongType) {
    EXPECT_FALSE(snapshot::decode({}, sizes()));
    Bytes bytes = encodeSample();
    bytes[0] = std::byte{0x31};
    EXPECT_FALSE(snapshot::decode(bytes, sizes()));
}

TEST(SnapshotDecoder, RejectsEveryTruncation) {
    const Bytes bytes = encodeSample();
    for (std::size_t length = 0; length < bytes.size(); ++length) {
        EXPECT_FALSE(snapshot::decode(std::span{bytes}.first(length), sizes())) << "length " << length;
    }
}

TEST(SnapshotDecoder, RejectsBytesLeftOver) {
    Bytes bytes = encodeSample();
    bytes.push_back(std::byte{0});
    EXPECT_FALSE(snapshot::decode(bytes, sizes()));
}

TEST(SnapshotDecoder, RejectsReservedFlags) {
    Bytes bytes = encodeSample();
    bytes[9] = std::byte{0x03};
    EXPECT_FALSE(snapshot::decode(bytes, sizes()));
    bytes[9] = std::byte{0x80};
    EXPECT_FALSE(snapshot::decode(bytes, sizes()));
    bytes[9] = std::byte{0x00};
    EXPECT_TRUE(snapshot::decode(bytes, sizes()));
}

TEST(SnapshotDecoder, RejectsCountsLargerThanTheBytesAllow) {
    Snapshot value;
    Bytes bytes = snapshot::encode(value).value();
    bytes[10] = std::byte{0xFF};  // 65 535 spawns in an 18-byte message
    bytes[11] = std::byte{0xFF};
    EXPECT_FALSE(snapshot::decode(bytes, sizes()));
}

TEST(SnapshotDecoder, RejectsAnUnknownShortId) {
    Snapshot value;
    value.spawns.push_back({.networkId = 1, .components = {{.shortId = 3, .bytes = makeBytes(4, 0)}}});
    EXPECT_FALSE(snapshot::decode(snapshot::encode(value).value(), sizes()));

    Snapshot removal;
    removal.removals.push_back({.networkId = 1, .shortId = 3});
    EXPECT_FALSE(snapshot::decode(snapshot::encode(removal).value(), sizes()));
}

TEST(SnapshotDecoder, RejectsAComponentListedTwiceInARecord) {
    Snapshot value;
    value.spawns.push_back(
        {.networkId = 1,
         .components = {{.shortId = 1, .bytes = makeBytes(1, 0)}, {.shortId = 1, .bytes = makeBytes(1, 1)}}});
    EXPECT_FALSE(snapshot::decode(snapshot::encode(value).value(), sizes()));

    Snapshot update;
    update.updates.push_back(
        {.networkId = 1,
         .components = {{.shortId = 1, .bytes = makeBytes(1, 0)}, {.shortId = 1, .bytes = makeBytes(1, 1)}}});
    EXPECT_FALSE(snapshot::decode(snapshot::encode(update).value(), sizes()));
}

TEST(SnapshotDecoder, RejectsANetIdListedTwiceInAList) {
    Snapshot spawns;
    spawns.spawns = {{.networkId = 2}, {.networkId = 2}};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(spawns).value(), sizes()));

    Snapshot updates;
    updates.updates = {{.networkId = 2}, {.networkId = 2}};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(updates).value(), sizes()));

    Snapshot despawns;
    despawns.despawns = {2, 2};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(despawns).value(), sizes()));
}

TEST(SnapshotDecoder, AcceptsTheSameNetIdInDifferentLists) {
    Snapshot value;
    value.spawns = {{.networkId = 2}};
    value.updates = {{.networkId = 2}};
    value.removals = {{.networkId = 2, .shortId = 0}, {.networkId = 2, .shortId = 1}};
    value.despawns = {2};
    EXPECT_EQ(snapshot::decode(snapshot::encode(value).value(), sizes()), value);
}

TEST(SnapshotDecoder, RejectsANetIdOfZero) {
    Snapshot spawn;
    spawn.spawns = {{.networkId = 0}};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(spawn).value(), sizes()));

    Snapshot update;
    update.updates = {{.networkId = 0}};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(update).value(), sizes()));

    Snapshot removal;
    removal.removals = {{.networkId = 0, .shortId = 0}};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(removal).value(), sizes()));

    Snapshot despawn;
    despawn.despawns = {0};
    EXPECT_FALSE(snapshot::decode(snapshot::encode(despawn).value(), sizes()));
}

TEST(SnapshotDecoder, NeverCrashesOnEveryByteOfAValidMessage) {
    const Bytes original = encodeSample();
    for (std::size_t index = 0; index < original.size(); ++index) {
        for (const std::uint8_t value : {0x00, 0x01, 0x7F, 0xFF}) {
            Bytes bytes = original;
            bytes[index] = static_cast<std::byte>(value);
            static_cast<void>(snapshot::decode(bytes, sizes()));
        }
    }
    SUCCEED();
}
