/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <variant>

#include "rtype/network/Types.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/packet/PacketCodec.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"

namespace {
using rtype::replication::exceptions::PacketException;
using namespace rtype::replication::packet;  // NOLINT(google-build-using-namespace): the module under test

PacketSchema buy() {
    return {.name = "game.Buy",
            .direction = PacketDirection::kToServer,
            .channel = rtype::network::Channel::kReliable,
            .fields = {{.name = "item", .type = PacketType::kString, .maxLength = 8},
                       {.name = "qty", .type = PacketType::kU8},
                       {.name = "gold", .type = PacketType::kI32},
                       {.name = "speed", .type = PacketType::kF32},
                       {.name = "paid", .type = PacketType::kBool}}};
}

PacketRegistry registryWith(PacketSchema schema) {
    PacketRegistry registry;
    registry.declare(std::move(schema));
    return registry;
}

PacketValues buyValues() { return {std::string{"laser"}, 3.0, -40.0, 1.5, true}; }
}  // namespace

TEST(Packet, RoundTripsThroughTheWire) {
    const PacketRegistry registry = registryWith(buy());
    const PacketSchema& schema = *registry.find("game.Buy");

    const auto decoded = decodePacket(registry, PacketDirection::kToServer, encodePacket(schema, buyValues()));

    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->name, "game.Buy");
    EXPECT_EQ(decoded->values, buyValues());
}

TEST(Packet, RoundTripsThroughTheText) {
    const PacketRegistry registry = registryWith(buy());
    const PacketSchema& schema = *registry.find("game.Buy");
    std::string error;

    const auto parsed = parseValues(schema, formatValues(schema, buyValues()), error);

    ASSERT_TRUE(parsed) << error;
    EXPECT_EQ(*parsed, buyValues());
}

TEST(Packet, TextWithABadValueIsRefused) {
    const PacketRegistry registry = registryWith(buy());
    const PacketSchema& schema = *registry.find("game.Buy");
    std::string error;

    EXPECT_FALSE(parseValues(schema, "5:laser3:2561:03:1.51:0", error));  // qty 256
    EXPECT_FALSE(parseValues(schema,
                             "9:laserlaser1:3"
                             "1:0"
                             "1:0"
                             "1:0",
                             error));  // string over maxLength
    EXPECT_NE(error.find("item"), std::string::npos);
    EXPECT_FALSE(parseValues(schema, "5:laser1:31:01:01:2", error));    // bool 2
    EXPECT_FALSE(parseValues(schema, "5:laser3:1.51:01:01:1", error));  // qty not whole
    EXPECT_FALSE(parseValues(schema, "5:laser1:31:01:03:nan1:1", error));
    EXPECT_FALSE(parseValues(schema, "5:laser1:31:01:01:11:1", error));  // extra data
    EXPECT_FALSE(parseValues(schema, "5:laser1:3", error));              // fields missing
    EXPECT_FALSE(parseValues(schema, "99:x", error));                    // cut short
}

TEST(Packet, TheWireRefusesWhatBreaksTheSchema) {
    const PacketRegistry registry = registryWith(buy());
    const PacketSchema& schema = *registry.find("game.Buy");
    const rtype::network::Bytes good = encodePacket(schema, buyValues());

    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToClients, good));  // wrong direction
    auto truncated = good;
    truncated.pop_back();
    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToServer, truncated));
    auto trailing = good;
    trailing.push_back(std::byte{0});
    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToServer, trailing));
    auto badBool = good;
    badBool.back() = std::byte{2};
    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToServer, badBool));
    auto otherHash = good;
    otherHash[2 + schema.name.size()] ^= std::byte{0xFF};
    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToServer, otherHash));
    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToServer, rtype::network::Bytes{}));
    EXPECT_FALSE(decodePacket(registry, PacketDirection::kToServer, rtype::network::Bytes(300, std::byte{0x32})));
}

TEST(Packet, ADifferentDeclarationHasADifferentHash) {
    PacketSchema other = buy();
    other.fields[1].type = PacketType::kU16;
    const PacketRegistry theirs = registryWith(other);
    const PacketRegistry ours = registryWith(buy());

    const auto message = encodePacket(*theirs.find("game.Buy"), buyValues());

    EXPECT_NE(theirs.find("game.Buy")->hash(), ours.find("game.Buy")->hash());
    EXPECT_FALSE(decodePacket(ours, PacketDirection::kToServer, message));
}

TEST(Packet, TheHashIsStable) {
    // Hard-coded: two builds must agree, or a client and a server of different versions would read each other.
    EXPECT_EQ(buy().hash(), buy().hash());
    PacketSchema renamed = buy();
    renamed.name = "game.Sell";
    EXPECT_NE(buy().hash(), renamed.hash());
}

TEST(Packet, ARegistryRefusesBadDeclarations) {
    PacketRegistry registry;
    registry.declare(buy());

    EXPECT_THROW(registry.declare(buy()), PacketException);  // taken
    PacketSchema badName = buy();
    badName.name = "1bad";
    EXPECT_THROW(registry.declare(badName), PacketException);
    PacketSchema twice = buy();
    twice.name = "game.Twice";
    twice.fields.push_back(twice.fields.front());
    EXPECT_THROW(registry.declare(twice), PacketException);
    PacketSchema sender = buy();
    sender.name = "game.Sender";
    sender.fields.front().name = "sender";
    EXPECT_THROW(registry.declare(sender), PacketException);
    PacketSchema huge = buy();
    huge.name = "game.Huge";
    huge.fields = {{.name = "text", .type = PacketType::kString, .maxLength = 1000}};
    EXPECT_THROW(registry.declare(huge), PacketException);  // the message would not fit the transport
    PacketSchema fits = huge;
    fits.name = "game.Fits";
    fits.fields.front().maxLength = 900;
    EXPECT_NO_THROW(registry.declare(fits));
}

TEST(Packet, AStringWithoutALengthTakesTheDefault) {
    PacketSchema schema = buy();
    schema.fields.front().maxLength = 0;

    const PacketRegistry registry = registryWith(schema);

    EXPECT_EQ(registry.find("game.Buy")->fields.front().maxLength, kDefaultStringSize);
}

TEST(Packet, AFrozenRegistryRefusesDeclarations) {
    PacketRegistry registry;
    registry.freeze();

    EXPECT_TRUE(registry.isFrozen());
    EXPECT_THROW(registry.declare(buy()), PacketException);
    EXPECT_EQ(registry.size(), 0U);
}
