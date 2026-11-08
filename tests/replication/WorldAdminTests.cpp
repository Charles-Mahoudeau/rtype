/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldAdminTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "ServerFixtures.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/entity/Name.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"
#include "rtype/replication/admin/WorldAdmin.hpp"

namespace {
namespace net = rtype::network::admin;
namespace ecs = rtype::ecs;
using rtype::test::server::Cargo;
using rtype::test::server::Follow;
using rtype::test::server::Position;

/// What a request got: a world reply, or an error code.
struct Answer {
    std::optional<net::WorldReply> reply;
    std::optional<net::ErrorCode> error;
};

struct Fixture {
    ecs::World world;
    ecs::Schedules schedules{world, ecs::RunMode::kServer};
    ecs::Commands commands{world};
    rtype::replication::admin::WorldAdmin admin{world, &schedules};

    Fixture() { admin.bind(&commands); }

    /// Sends a request through the wire form and applies the changes it recorded.
    Answer ask(const net::WorldRequest& request, const bool apply = true) {
        const std::optional<net::Reply> reply = admin.handle(net::encodeWorldRequest(request));
        Answer answer;
        if (!reply) {
            return answer;
        }
        if (const auto* failed = std::get_if<net::ErrorReply>(&*reply)) {
            answer.error = failed->code;
        } else {
            answer.reply = net::decodeWorldReply(std::get<net::ExtensionReply>(*reply));
            EXPECT_TRUE(answer.reply.has_value());
        }
        if (apply) {
            commands.flush();
        }
        return answer;
    }

    ecs::Entity spawnShip(const std::string& name, const float x = 1.5F) {
        const ecs::Entity entity = world.spawn();
        world.getComponents().add(entity, ecs::Name{.value = ecs::String{name}});
        world.getComponents().add(entity, Position{.x = x, .y = 2.F});
        return entity;
    }
};

template <typename T>
T as(const Answer& answer) {
    EXPECT_TRUE(answer.reply.has_value());
    return answer.reply ? std::get<T>(*answer.reply) : T{};
}
}  // namespace

TEST(WorldAdmin, ListsTheEntitiesThatHaveAComponentWithTheirNames) {
    Fixture fixture;
    const ecs::Entity ship = fixture.spawnShip("Ship");
    static_cast<void>(fixture.world.spawn());  // no component: not listed
    const ecs::Entity other = fixture.spawnShip("Rock");

    const Answer answer = fixture.ask(net::EntitiesRequest{});
    const auto page = as<net::EntitiesReply>(answer);
    EXPECT_EQ(page.total, 2U);
    ASSERT_EQ(page.entries.size(), 2U);
    EXPECT_EQ(page.entries.at(0).entity, ship.getBits());
    EXPECT_EQ(page.entries.at(0).name, "Ship");
    EXPECT_EQ(page.entries.at(0).componentCount, 2U);
    EXPECT_EQ(page.entries.at(1).entity, other.getBits());
}

TEST(WorldAdmin, PagesTheEntityList) {
    Fixture fixture;
    for (int i = 0; i < 60; ++i) {
        fixture.spawnShip(std::string(50, 'a'));
    }
    const auto first = as<net::EntitiesReply>(fixture.ask(net::EntitiesRequest{.first = 0}));
    EXPECT_EQ(first.total, 60U);
    EXPECT_GT(first.entries.size(), 5U);
    EXPECT_LT(first.entries.size(), 60U);  // a datagram holds a page

    std::size_t seen = 0;
    std::uint32_t next = 0;
    while (seen < 60) {
        const Answer answer = fixture.ask(net::EntitiesRequest{.first = next});
        const auto page = as<net::EntitiesReply>(answer);
        ASSERT_FALSE(page.entries.empty());
        seen += page.entries.size();
        next = page.first + static_cast<std::uint32_t>(page.entries.size());
    }
    EXPECT_EQ(seen, 60U);
    // Past the end: an empty page, not an error.
    EXPECT_TRUE(as<net::EntitiesReply>(fixture.ask(net::EntitiesRequest{.first = 1000})).entries.empty());
}

TEST(WorldAdmin, InspectsTheComponentsAndValuesOfAnEntity) {
    Fixture fixture;
    const ecs::Entity ship = fixture.spawnShip("Ship", 1.5F);
    const Answer answer = fixture.ask(net::InspectRequest{.entity = ship.getBits()});
    const auto view = as<net::InspectReply>(answer);
    EXPECT_EQ(view.name, "Ship");
    EXPECT_EQ(view.total, 2U);
    EXPECT_EQ(view.next, 2U);
    bool found = false;
    for (const net::ComponentView& component : view.components) {
        if (component.name != "test.server.Position") {
            continue;
        }
        found = true;
        ASSERT_EQ(component.fields.size(), 2U);
        EXPECT_EQ(component.fields.at(0).name, "x");
        EXPECT_EQ(component.fields.at(0).type, "f32");
        EXPECT_EQ(component.fields.at(0).value, "1.5");
        EXPECT_TRUE(component.fields.at(0).editable);
    }
    EXPECT_TRUE(found);
}

TEST(WorldAdmin, ShowsAnEntityFieldAsReadOnlyAndAListAsItsLength) {
    Fixture fixture;
    const ecs::Entity entity = fixture.world.spawn();
    fixture.world.getComponents().add(entity, Follow{.target = entity});
    fixture.world.getComponents().add(entity, Cargo{.items = {1, 2, 3}, .crew = {}, .weight = 4.F});
    const auto view = as<net::InspectReply>(fixture.ask(net::InspectRequest{.entity = entity.getBits()}));
    for (const net::ComponentView& component : view.components) {
        if (component.name == "test.server.Follow") {
            EXPECT_EQ(component.fields.front().type, "entity");
            EXPECT_FALSE(component.fields.front().editable);
        } else if (component.name == "test.server.Cargo") {
            EXPECT_EQ(component.fields.front().value, "<3 items>");
            EXPECT_FALSE(component.fields.front().editable);
        }
    }
}

TEST(WorldAdmin, RefusesAnEntityThatIsNotAlive) {
    Fixture fixture;
    const ecs::Entity ship = fixture.spawnShip("Ship");
    EXPECT_EQ(fixture.ask(net::InspectRequest{.entity = 0}).error, net::ErrorCode::kUnknownEntity);
    EXPECT_EQ(fixture.ask(net::InspectRequest{.entity = 0xFFFFFFFFFFFFFFFFULL}).error, net::ErrorCode::kUnknownEntity);
    EXPECT_EQ(fixture.ask(net::DespawnRequest{.entity = 12345}).error, net::ErrorCode::kUnknownEntity);
    EXPECT_EQ(fixture.ask(net::SetFieldRequest{.entity = 12345, .component = "x", .field = "y", .value = "1"}).error,
              net::ErrorCode::kUnknownEntity);
    fixture.world.despawn(ship);
    EXPECT_EQ(fixture.ask(net::InspectRequest{.entity = ship.getBits()}).error, net::ErrorCode::kUnknownEntity);
}

TEST(WorldAdmin, SetsAFieldBetweenFramesNotImmediately) {
    Fixture fixture;
    const ecs::Entity ship = fixture.spawnShip("Ship", 1.F);
    const Answer answer = fixture.ask(
        net::SetFieldRequest{
            .entity = ship.getBits(), .component = "test.server.Position", .field = "x", .value = " -3.25 "},
        false);
    EXPECT_TRUE(answer.reply.has_value());
    EXPECT_EQ(fixture.world.getComponents().get<Position>(ship)->x, 1.F);  // recorded, not applied yet
    fixture.commands.flush();
    EXPECT_EQ(fixture.world.getComponents().get<Position>(ship)->x, -3.25F);
    EXPECT_EQ(fixture.world.getComponents().get<Position>(ship)->y, 2.F);  // the rest untouched
}

TEST(WorldAdmin, SetsAStringField) {
    Fixture fixture;
    const ecs::Entity ship = fixture.spawnShip("Ship");
    static_cast<void>(fixture.ask(net::SetFieldRequest{
        .entity = ship.getBits(), .component = "rtype.Name", .field = "value", .value = "Flagship"}));
    EXPECT_EQ(std::string{fixture.world.getComponents().get<ecs::Name>(ship)->value.view()}, "Flagship");
}

TEST(WorldAdmin, RejectsBadSetRequestsWithoutChangingAnything) {
    Fixture fixture;
    const ecs::Entity ship = fixture.spawnShip("Ship", 1.F);
    const auto set = [&](const std::string& component, const std::string& field, const std::string& value,
                         const std::uint16_t index = 0) {
        return fixture
            .ask(net::SetFieldRequest{
                .entity = ship.getBits(), .component = component, .field = field, .index = index, .value = value})
            .error;
    };
    EXPECT_EQ(set("test.server.Nope", "x", "1"), net::ErrorCode::kUnknownComponent);
    EXPECT_EQ(set("test.server.Follow", "target", "1"), net::ErrorCode::kUnknownComponent);  // the entity has none
    EXPECT_EQ(set("test.server.Position", "z", "1"), net::ErrorCode::kUnknownField);
    EXPECT_EQ(set("test.server.Position", "x", "1", 1), net::ErrorCode::kUnknownField);  // not an array
    EXPECT_EQ(set("test.server.Position", "x", "abc"), net::ErrorCode::kBadValue);
    EXPECT_EQ(set("test.server.Position", "x", ""), net::ErrorCode::kBadValue);
    EXPECT_EQ(set("test.server.Position", "x", "nan"), net::ErrorCode::kBadValue);
    EXPECT_EQ(set("test.server.Position", "x", "inf"), net::ErrorCode::kBadValue);
    EXPECT_EQ(set("test.server.Position", "x", "1e999"), net::ErrorCode::kBadValue);
    EXPECT_EQ(set("test.server.Position", "x", "1 2"), net::ErrorCode::kBadValue);
    EXPECT_EQ(fixture.world.getComponents().get<Position>(ship)->x, 1.F);
}

TEST(WorldAdmin, RefusesToEditAnEntityFieldOrAList) {
    Fixture fixture;
    const ecs::Entity entity = fixture.world.spawn();
    fixture.world.getComponents().add(entity, Follow{.target = entity});
    fixture.world.getComponents().add(entity, Cargo{.items = {1}, .crew = {}, .weight = 0.F});
    EXPECT_EQ(fixture
                  .ask(net::SetFieldRequest{
                      .entity = entity.getBits(), .component = "test.server.Follow", .field = "target", .value = "1"})
                  .error,
              net::ErrorCode::kBadValue);
    EXPECT_EQ(fixture
                  .ask(net::SetFieldRequest{
                      .entity = entity.getBits(), .component = "test.server.Cargo", .field = "items", .value = "1"})
                  .error,
              net::ErrorCode::kBadValue);
}

TEST(WorldAdmin, SpawnsAnEntityWithANameAndDespawnsIt) {
    Fixture fixture;
    const Answer spawned = fixture.ask(net::SpawnRequest{.name = "Boss"});
    const std::uint64_t bits = as<net::SpawnReply>(spawned).entity;
    const auto list = as<net::EntitiesReply>(fixture.ask(net::EntitiesRequest{}));
    ASSERT_EQ(list.entries.size(), 1U);
    EXPECT_EQ(list.entries.front().entity, bits);
    EXPECT_EQ(list.entries.front().name, "Boss");

    EXPECT_EQ(as<net::SpawnReply>(fixture.ask(net::SpawnRequest{})).entity != bits, true);
    EXPECT_EQ(as<net::DespawnReply>(fixture.ask(net::DespawnRequest{.entity = bits})).entity, bits);
    EXPECT_EQ(as<net::EntitiesReply>(fixture.ask(net::EntitiesRequest{})).total, 1U);  // "Entity" is left
    EXPECT_EQ(fixture.ask(net::DespawnRequest{.entity = bits}).error, net::ErrorCode::kUnknownEntity);
}

TEST(WorldAdmin, PausesResumesAndStepsTheSchedules) {
    Fixture fixture;
    EXPECT_FALSE(as<net::RunReply>(fixture.ask(net::RunRequest{.action = net::RunAction::kQuery})).paused);
    EXPECT_TRUE(as<net::RunReply>(fixture.ask(net::RunRequest{.action = net::RunAction::kPause})).paused);
    EXPECT_TRUE(fixture.schedules.isPaused());
    EXPECT_TRUE(as<net::RunReply>(fixture.ask(net::RunRequest{.action = net::RunAction::kStep})).paused);
    EXPECT_FALSE(as<net::RunReply>(fixture.ask(net::RunRequest{.action = net::RunAction::kResume})).paused);
    EXPECT_FALSE(fixture.schedules.isPaused());
}

TEST(WorldAdmin, RefusesChangesWithoutCommandsAndPauseWithoutSchedules) {
    ecs::World world;
    rtype::replication::admin::WorldAdmin admin{world};  // no schedules, nothing bound
    const auto error = [&](const net::WorldRequest& request) {
        const auto reply = admin.handle(net::encodeWorldRequest(request));
        return reply && std::holds_alternative<net::ErrorReply>(*reply) ? std::get<net::ErrorReply>(*reply).code
                                                                        : net::ErrorCode::kMalformed;
    };
    EXPECT_EQ(error(net::SpawnRequest{}), net::ErrorCode::kUnsupported);
    EXPECT_EQ(error(net::DespawnRequest{.entity = 1}), net::ErrorCode::kUnsupported);
    EXPECT_EQ(error(net::SetFieldRequest{.entity = 1}), net::ErrorCode::kUnsupported);
    EXPECT_EQ(error(net::RunRequest{.action = net::RunAction::kPause}), net::ErrorCode::kUnsupported);
    // Reads need neither.
    EXPECT_TRUE(admin.handle(net::encodeWorldRequest(net::EntitiesRequest{})).has_value());
}

TEST(WorldAdmin, AnswersMalformedAndUnknownRequestsWithoutCrashing) {
    Fixture fixture;
    const auto bad = fixture.admin.handle({.type = 0x21, .body = {std::byte{1}}});  // truncated inspect
    ASSERT_TRUE(bad.has_value());
    EXPECT_EQ(std::get<net::ErrorReply>(*bad).code, net::ErrorCode::kMalformed);
    EXPECT_FALSE(fixture.admin.handle({.type = 0x30, .body = {}}).has_value());  // not a world type: caller refuses
    EXPECT_FALSE(fixture.admin.handle({.type = 0x26, .body = {}}).has_value());
}

TEST(WorldAdmin, AComponentTooBigForOneReplyIsContinuedInTheNext) {
    Fixture fixture;
    const ecs::Entity entity = fixture.world.spawn();
    fixture.world.getComponents().add(entity, Position{});
    fixture.world.getComponents().add(entity, Follow{});
    fixture.world.getComponents().add(entity, Cargo{});
    fixture.world.getComponents().add(entity, ecs::Name{.value = ecs::String{"Many"}});
    std::uint16_t next = 0;
    std::size_t components = 0;
    do {
        const auto& view =
            as<net::InspectReply>(fixture.ask(net::InspectRequest{.entity = entity.getBits(), .firstComponent = next}));
        ASSERT_GT(view.next, next);
        components += view.components.size();
        next = view.next;
        if (next >= view.total) {
            break;
        }
    } while (true);
    EXPECT_EQ(components, 4U);
}
