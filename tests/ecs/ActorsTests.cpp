/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::actors {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.actors.Health", value);
}  // namespace rtype::test::actors

namespace pre = rtype::test::actors;

namespace {
/// @return The message of the ActorException `call` throws, or an empty string if it throws none.
template <typename Call>
std::string errorOf(Call call) {
    try {
        call();
    } catch (const rtype::ecs::exceptions::ActorException& error) {
        return error.what();
    }
    return "";
}

/// @return A type of this presence, for the filter.
rtype::ecs::TypeInfo typeOf(rtype::ecs::Presence presence) {
    return rtype::ecs::TypeInfo{.name = "test.actors.Type", .presence = presence};
}
}  // namespace

TEST(Actors, AWorldStartsWithNoActors) {
    const rtype::ecs::World world;
    EXPECT_EQ(world.getActors().getCount(), 0U);
    EXPECT_EQ(world.getActors().findByName("game.Bydo"), nullptr);
    EXPECT_EQ(world.getActors().findById(0), nullptr);
    EXPECT_EQ(world.getActors().getRunMode(), rtype::ecs::RunMode::kStandalone);
}

TEST(Actors, AddsAActorFoundByIdAndByName) {
    rtype::ecs::World world;
    const rtype::ecs::ActorId bydo = world.getActors().add("game.Bydo");
    const rtype::ecs::ActorId bullet = world.getActors().add("game.Bullet");
    EXPECT_EQ(bydo, 0U);
    EXPECT_EQ(bullet, 1U);
    ASSERT_NE(world.getActors().findByName("game.Bullet"), nullptr);
    EXPECT_EQ(world.getActors().findByName("game.Bullet")->getId(), bullet);
    EXPECT_EQ(world.getActors().findById(bydo)->getName(), "game.Bydo");
    EXPECT_EQ(world.getActors().findById(bydo)->getValues().getCount(), 0U);
    EXPECT_TRUE(world.getActors().findById(bydo)->getChildren().empty());
}

TEST(Actors, EditsAActorsValues) {
    rtype::ecs::World world;
    const rtype::ecs::ActorId bydo = world.getActors().add("game.Bydo");
    world.getActors().edit(bydo).getValues().set(world.getComponents(), pre::Health{.value = 3});
    const rtype::ecs::Actor* actor = world.getActors().findById(bydo);
    ASSERT_NE(actor->getValues().get<pre::Health>(world.getComponents()), nullptr);
    EXPECT_EQ(actor->getValues().get<pre::Health>(world.getComponents())->value, 3);
}

TEST(Actors, RefusesAnEmptyOrTakenName) {
    rtype::ecs::World world;
    static_cast<void>(world.getActors().add("game.Bydo"));
    EXPECT_EQ(errorOf([&] { static_cast<void>(world.getActors().add("")); }), "An actor needs a name");
    EXPECT_EQ(errorOf([&] { static_cast<void>(world.getActors().add("game.Bydo")); }),
              "Actor 'game.Bydo' is already registered");
    EXPECT_EQ(world.getActors().getCount(), 1U);
}

TEST(Actors, RefusesToEditAnUnknownActor) {
    rtype::ecs::World world;
    EXPECT_EQ(errorOf([&] { static_cast<void>(world.getActors().edit(4)); }), "No actor has the id 4");
}

TEST(Actors, AddsChildrenWithTheirOverrides) {
    rtype::ecs::World world;
    rtype::ecs::Actors& actors = world.getActors();
    const rtype::ecs::ActorId boss = actors.add("game.Boss");
    const rtype::ecs::ActorId turret = actors.add("game.Turret");
    rtype::ecs::ComponentValues overrides;
    overrides.set(world.getComponents(), pre::Health{.value = 9});
    actors.addChild(boss, turret, std::move(overrides));
    actors.addChild(boss, turret);
    const auto children = actors.findById(boss)->getChildren();
    ASSERT_EQ(children.size(), 2U);
    EXPECT_EQ(children[0].actor, turret);
    EXPECT_EQ(children[0].overrides.get<pre::Health>(world.getComponents())->value, 9);
    EXPECT_EQ(children[1].overrides.getCount(), 0U);
}

TEST(Actors, RefusesAChildWithAnUnknownId) {
    rtype::ecs::World world;
    const rtype::ecs::ActorId boss = world.getActors().add("game.Boss");
    EXPECT_EQ(errorOf([&] { world.getActors().addChild(boss, 7); }), "No actor has the id 7");
    EXPECT_EQ(errorOf([&] { world.getActors().addChild(7, boss); }), "No actor has the id 7");
}

TEST(Actors, RefusesAActorContainingItself) {
    rtype::ecs::World world;
    rtype::ecs::Actors& actors = world.getActors();
    const rtype::ecs::ActorId a = actors.add("game.A");
    const rtype::ecs::ActorId b = actors.add("game.B");
    const rtype::ecs::ActorId c = actors.add("game.C");
    actors.addChild(a, b);
    actors.addChild(b, c);
    EXPECT_EQ(errorOf([&] { actors.addChild(a, a); }),
              "Actor 'game.A' cannot have 'game.A' as a child: 'game.A' contains 'game.A'");
    EXPECT_EQ(errorOf([&] { actors.addChild(c, a); }),
              "Actor 'game.C' cannot have 'game.A' as a child: 'game.A' contains 'game.C'");
    EXPECT_TRUE(actors.findById(c)->getChildren().empty());
    actors.addChild(a, c);  // a diamond is not a cycle
    EXPECT_EQ(actors.findById(a)->getChildren().size(), 2U);
}

TEST(Actors, KeepsTheComponentsOfItsSide) {
    rtype::ecs::Actors actors;
    const rtype::ecs::TypeInfo both = typeOf(rtype::ecs::Presence::kBoth);
    const rtype::ecs::TypeInfo server = typeOf(rtype::ecs::Presence::kServerOnly);
    const rtype::ecs::TypeInfo client = typeOf(rtype::ecs::Presence::kClientOnly);
    EXPECT_TRUE(actors.keeps(both) && actors.keeps(server) && actors.keeps(client));  // standalone
    actors.setRunMode(rtype::ecs::RunMode::kClient);
    EXPECT_TRUE(actors.keeps(both));
    EXPECT_FALSE(actors.keeps(server));
    EXPECT_TRUE(actors.keeps(client));
    actors.setRunMode(rtype::ecs::RunMode::kServer);
    EXPECT_TRUE(actors.keeps(both));
    EXPECT_TRUE(actors.keeps(server));
    EXPECT_FALSE(actors.keeps(client));
    EXPECT_EQ(actors.getRunMode(), rtype::ecs::RunMode::kServer);
}
