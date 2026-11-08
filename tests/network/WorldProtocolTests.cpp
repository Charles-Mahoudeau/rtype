/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldProtocolTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"

namespace {
namespace admin = rtype::network::admin;

/// @return @p request encoded, decoded and checked to be of type @p T.
template <typename T>
T requestRoundTrip(const admin::WorldRequest& request) {
    const admin::ExtensionRequest wire = admin::encodeWorldRequest(request);
    const auto decoded = admin::decodeWorldRequest(wire);
    EXPECT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded && std::holds_alternative<T>(*decoded));
    return decoded ? std::get<T>(*decoded) : T{};
}

/// @return @p reply encoded, decoded and checked to be of type @p T.
template <typename T>
T replyRoundTrip(const admin::WorldReply& reply) {
    const admin::ExtensionReply wire = admin::encodeWorldReply(reply);
    EXPECT_NE(wire.type & admin::kReplyFlag, 0);
    const auto decoded = admin::decodeWorldReply(wire);
    EXPECT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded && std::holds_alternative<T>(*decoded));
    return decoded ? std::get<T>(*decoded) : T{};
}
}  // namespace

TEST(WorldProtocol, RequestsKeepTheirContent) {
    EXPECT_EQ(requestRoundTrip<admin::EntitiesRequest>(admin::EntitiesRequest{.first = 42}).first, 42U);
    const auto inspect = requestRoundTrip<admin::InspectRequest>(
        admin::InspectRequest{.entity = 0x0000000500000007ULL, .firstComponent = 3});
    EXPECT_EQ(inspect.entity, 0x0000000500000007ULL);
    EXPECT_EQ(inspect.firstComponent, 3U);
    const auto set = requestRoundTrip<admin::SetFieldRequest>(admin::SetFieldRequest{
        .entity = 9, .component = "rtype.Transform", .field = "position", .index = 2, .value = "1 2 3"});
    EXPECT_EQ(set.entity, 9U);
    EXPECT_EQ(set.component, "rtype.Transform");
    EXPECT_EQ(set.field, "position");
    EXPECT_EQ(set.index, 2U);
    EXPECT_EQ(set.value, "1 2 3");
    EXPECT_EQ(requestRoundTrip<admin::SpawnRequest>(admin::SpawnRequest{.name = "Boss"}).name, "Boss");
    EXPECT_EQ(requestRoundTrip<admin::DespawnRequest>(admin::DespawnRequest{.entity = 11}).entity, 11U);
    EXPECT_EQ(requestRoundTrip<admin::RunRequest>(admin::RunRequest{.action = admin::RunAction::kStep}).action,
              admin::RunAction::kStep);
}

TEST(WorldProtocol, RequestTypesAreInTheWorldRange) {
    EXPECT_EQ(admin::encodeWorldRequest(admin::EntitiesRequest{}).type, 0x20);
    EXPECT_EQ(admin::encodeWorldRequest(admin::RunRequest{}).type, 0x25);
    EXPECT_GE(admin::encodeWorldRequest(admin::SpawnRequest{}).type, admin::kWorldFirst);
    EXPECT_LE(admin::encodeWorldRequest(admin::DespawnRequest{}).type, admin::kExtensionLast);
}

TEST(WorldProtocol, LongNamesAndValuesAreCutNotRejected) {
    const auto set = requestRoundTrip<admin::SetFieldRequest>(admin::SetFieldRequest{.entity = 1,
                                                                                     .component = std::string(300, 'c'),
                                                                                     .field = std::string(100, 'f'),
                                                                                     .value = std::string(500, 'v')});
    EXPECT_EQ(set.component.size(), admin::kMaxWorldName);
    EXPECT_EQ(set.field.size(), admin::kMaxWorldName);
    EXPECT_EQ(set.value.size(), admin::kMaxWorldValue);
}

TEST(WorldProtocol, RepliesKeepTheirContent) {
    admin::EntitiesReply entities{.total = 20, .first = 5};
    entities.entries.push_back({.entity = 77, .name = "Ship", .componentCount = 4});
    entities.entries.push_back({.entity = 78, .name = "", .componentCount = 1});
    const auto page = replyRoundTrip<admin::EntitiesReply>(entities);
    EXPECT_EQ(page.total, 20U);
    EXPECT_EQ(page.first, 5U);
    ASSERT_EQ(page.entries.size(), 2U);
    EXPECT_EQ(page.entries.at(0).entity, 77U);
    EXPECT_EQ(page.entries.at(0).name, "Ship");
    EXPECT_EQ(page.entries.at(0).componentCount, 4U);
    EXPECT_TRUE(page.entries.at(1).name.empty());

    admin::InspectReply inspect{.entity = 77, .name = "Ship", .total = 3, .next = 2};
    inspect.components.push_back(
        {.name = "rtype.Transform",
         .fields = {{.name = "x", .type = "f32", .count = 1, .value = "1.5", .editable = true},
                    {.name = "id", .type = "entity", .count = 1, .value = "0", .editable = false}}});
    const auto view = replyRoundTrip<admin::InspectReply>(inspect);
    EXPECT_EQ(view.entity, 77U);
    EXPECT_EQ(view.name, "Ship");
    EXPECT_EQ(view.total, 3U);
    EXPECT_EQ(view.next, 2U);
    ASSERT_EQ(view.components.size(), 1U);
    ASSERT_EQ(view.components.front().fields.size(), 2U);
    EXPECT_EQ(view.components.front().fields.at(0).value, "1.5");
    EXPECT_TRUE(view.components.front().fields.at(0).editable);
    EXPECT_FALSE(view.components.front().fields.at(1).editable);

    static_cast<void>(replyRoundTrip<admin::SetFieldReply>(admin::SetFieldReply{}));
    EXPECT_EQ(replyRoundTrip<admin::SpawnReply>(admin::SpawnReply{.entity = 5}).entity, 5U);
    EXPECT_EQ(replyRoundTrip<admin::DespawnReply>(admin::DespawnReply{.entity = 6}).entity, 6U);
    EXPECT_TRUE(replyRoundTrip<admin::RunReply>(admin::RunReply{.paused = true}).paused);
}

TEST(WorldProtocol, AValueTooLongForAFieldIsCutWithDots) {
    admin::InspectReply inspect{.entity = 1};
    inspect.components.push_back(
        {.name = "c", .fields = {{.name = "f", .type = "string", .value = std::string(300, 'z')}}});
    const auto view = replyRoundTrip<admin::InspectReply>(inspect);
    const std::string& value = view.components.front().fields.front().value;
    EXPECT_EQ(value.size(), admin::kMaxWorldValue);
    EXPECT_EQ(value.substr(value.size() - 3), "...");
}

TEST(WorldProtocol, SizesHelpersMatchTheEncoding) {
    admin::EntitiesReply entities{.total = 1, .first = 0};
    entities.entries.push_back({.entity = 1, .name = "abc", .componentCount = 1});
    const admin::ExtensionReply wire = admin::encodeWorldReply(entities);
    EXPECT_EQ(wire.body.size(), admin::entitiesPrefixSize() + admin::encodedSize(entities.entries.front()));

    admin::InspectReply inspect{.entity = 1, .name = "abc"};
    inspect.components.push_back({.name = "comp", .fields = {{.name = "f", .type = "f32", .value = "1"}}});
    const admin::ExtensionReply inspected = admin::encodeWorldReply(inspect);
    EXPECT_EQ(inspected.body.size(), admin::inspectPrefixSize(3) + admin::encodedSize(inspect.components.front()));
}

TEST(WorldProtocol, ATooBigReplyBecomesAnEmptyBodyThatDoesNotDecode) {
    admin::EntitiesReply entities{.total = 100, .first = 0};
    for (int i = 0; i < 100; ++i) {
        entities.entries.push_back(
            {.entity = static_cast<std::uint64_t>(i), .name = std::string(60, 'n'), .componentCount = 1});
    }
    const admin::ExtensionReply wire = admin::encodeWorldReply(entities);
    EXPECT_TRUE(wire.body.empty());
    EXPECT_FALSE(admin::decodeWorldReply(wire).has_value());
}

TEST(WorldProtocol, MalformedRequestsAreRejected) {
    admin::ExtensionRequest wire = admin::encodeWorldRequest(admin::InspectRequest{.entity = 1});
    wire.body.pop_back();  // truncated
    EXPECT_FALSE(admin::decodeWorldRequest(wire).has_value());
    wire = admin::encodeWorldRequest(admin::InspectRequest{.entity = 1});
    wire.body.push_back(std::byte{0});  // trailing byte
    EXPECT_FALSE(admin::decodeWorldRequest(wire).has_value());
    EXPECT_FALSE(admin::decodeWorldRequest({.type = 0x26, .body = {}}).has_value());  // not a world type
    EXPECT_FALSE(admin::decodeWorldRequest({.type = 0x01, .body = {}}).has_value());
    EXPECT_FALSE(admin::decodeWorldRequest({.type = 0x25, .body = {std::byte{9}}}).has_value());  // bad action
    // A string whose length runs past the end.
    EXPECT_FALSE(admin::decodeWorldRequest({.type = 0x23, .body = {std::byte{200}}}).has_value());
}

TEST(WorldProtocol, MalformedRepliesAreRejected) {
    EXPECT_FALSE(admin::decodeWorldReply({.type = 0x20, .body = {}}).has_value());  // no reply flag
    EXPECT_FALSE(admin::decodeWorldReply({.type = 0xA6, .body = {}}).has_value());  // not a world type
    // An entities reply that claims 65535 entries in a few bytes.
    admin::ExtensionReply wire{.type = 0xA0, .body = std::vector<std::byte>(10, std::byte{0xFF})};
    EXPECT_FALSE(admin::decodeWorldReply(wire).has_value());
    // An inspect reply that claims 255 components with a few bytes left.
    admin::ExtensionReply inspect = admin::encodeWorldReply(admin::InspectReply{.entity = 1});
    ASSERT_FALSE(inspect.body.empty());
    inspect.body.back() = std::byte{255};
    EXPECT_FALSE(admin::decodeWorldReply(inspect).has_value());
}

TEST(WorldProtocol, EveryTruncationOfAReplyIsRejectedWithoutCrashing) {
    admin::InspectReply inspect{.entity = 3, .name = "n", .total = 1, .next = 1};
    inspect.components.push_back({.name = "c", .fields = {{.name = "f", .type = "f32", .value = "1"}}});
    const admin::ExtensionReply wire = admin::encodeWorldReply(inspect);
    for (std::size_t length = 0; length < wire.body.size(); ++length) {
        admin::ExtensionReply cut{.type = wire.type, .body = {wire.body.begin(), wire.body.begin() + length}};
        EXPECT_FALSE(admin::decodeWorldReply(cut).has_value()) << "length " << length;
    }
}

TEST(WorldProtocol, WorldErrorCodesTravelInErrorReplies) {
    for (const auto code : {admin::ErrorCode::kUnknownEntity, admin::ErrorCode::kUnknownComponent,
                            admin::ErrorCode::kUnknownField, admin::ErrorCode::kBadValue}) {
        const auto bytes = admin::encodeReply(1, admin::ErrorReply{.code = code});
        const auto decoded = admin::decodeReply(bytes);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(std::get<admin::ErrorReply>(decoded->reply).code, code);
    }
}
