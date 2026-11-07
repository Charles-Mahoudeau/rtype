/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandsPatchTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::patch {
struct Position {
    float x{0.F};
    float y{0.F};
};
RTYPE_ECS_COMPONENT(Position, "test.patch.Position", x, y);

struct Health {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Health, "test.patch.Health", value);

struct Named {
    std::string name;
    std::int32_t rank{0};
};
RTYPE_ECS_COMPONENT(Named, "test.patch.Named");

struct Frozen {};
RTYPE_ECS_COMPONENT(Frozen, "test.patch.Frozen");

/// @brief Registered only by the flush of the commands that use it.
struct Late {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Late, "test.patch.Late", value);
}  // namespace rtype::test::patch

namespace ecs = rtype::ecs;
namespace tp = rtype::test::patch;

TEST(CommandsPatch, KeepsWhatItDoesNotWrite) {
    ecs::World world;
    ecs::Commands commands{world};
    const ecs::Entity entity = commands.spawn()
                                   .add(tp::Position{.x = 1.F, .y = 2.F})
                                   .patch<tp::Position>([](tp::Position& position) { position.x = 5.F; })
                                   .getEntity();
    commands.flush();
    const tp::Position* position = world.getComponents().get<tp::Position>(entity);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->x, 5.F);
    EXPECT_FLOAT_EQ(position->y, 2.F);  // add() would have reset it
}

TEST(CommandsPatch, ReadsTheValueTheCommandsBeforeItLeft) {
    ecs::World world;
    const ecs::Entity entity = world.spawn();
    static_cast<void>(world.getComponents().add(entity, tp::Health{.value = 1}));
    ecs::Commands commands{world};
    commands.add(entity, tp::Health{.value = 10});
    commands.patch<tp::Health>(entity, [](tp::Health& health) { health.value += 5; });
    commands.patch<tp::Health>(entity, [](tp::Health& health) { health.value *= 2; });
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 1);  // nothing before the flush
    commands.flush();
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 30);
}

TEST(CommandsPatch, ACommandAfterItWins) {
    ecs::World world;
    ecs::Commands commands{world};
    const ecs::Entity entity = commands.spawn()
                                   .add(tp::Health{.value = 1})
                                   .patch<tp::Health>([](tp::Health& health) { health.value = 7; })
                                   .add(tp::Health{.value = 3})
                                   .getEntity();
    commands.flush();
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 3);
}

TEST(CommandsPatch, EditsAValueThatOwnsMemory) {
    ecs::World world;
    ecs::Commands commands{world};
    const ecs::Entity entity = commands.spawn()
                                   .add(tp::Named{.name = "bydo", .rank = 2})
                                   .patch<tp::Named>([](tp::Named& named) { named.name += " leader"; })
                                   .getEntity();
    commands.flush();
    const tp::Named* named = world.getComponents().get<tp::Named>(entity);
    ASSERT_NE(named, nullptr);
    EXPECT_EQ(named->name, "bydo leader");
    EXPECT_EQ(named->rank, 2);
}

TEST(CommandsPatch, SkipsADeadEntityAMissingComponentAndATag) {
    ecs::World world;
    const ecs::Entity dead = world.spawn();
    static_cast<void>(world.getComponents().add(dead, tp::Health{.value = 1}));
    const ecs::Entity bare = world.spawn();
    const ecs::Entity frozen = world.spawn();
    static_cast<void>(world.getComponents().add(frozen, tp::Frozen{}));
    std::int32_t runs = 0;
    ecs::Commands commands{world};
    commands.despawn(dead);
    commands.patch<tp::Health>(dead, [&runs](tp::Health& /*health*/) { ++runs; });
    commands.patch<tp::Health>(bare, [&runs](tp::Health& /*health*/) { ++runs; });
    commands.patch<tp::Frozen>(frozen, [&runs](tp::Frozen& /*frozen*/) { ++runs; });
    commands.flush();
    EXPECT_EQ(runs, 0);
    EXPECT_FALSE(world.getComponents().has<tp::Health>(bare));
    EXPECT_TRUE(world.getComponents().has<tp::Frozen>(frozen));
}

TEST(CommandsPatch, MarksTheValueChanged) {
    ecs::World world;
    const ecs::Entity entity = world.spawn();
    static_cast<void>(world.getComponents().add(entity, tp::Health{.value = 1}));
    const std::optional<ecs::ComponentTicks> before = world.getComponents().getTicks<tp::Health>(entity);
    ASSERT_TRUE(before);
    static_cast<void>(world.advanceChangeTick());
    ecs::Commands commands{world};
    commands.patch<tp::Health>(entity, [](tp::Health& health) { health.value = 2; });
    commands.flush();
    const std::optional<ecs::ComponentTicks> after = world.getComponents().getTicks<tp::Health>(entity);
    ASSERT_TRUE(after);
    EXPECT_GT(after->changed, before->changed);
    EXPECT_EQ(after->added, before->added);
}

TEST(CommandsPatch, RegistersATypeAddedInTheSameBuffer) {
    ecs::World world;
    ecs::Commands commands{world};
    const ecs::Entity entity =
        commands.spawn().add(tp::Late{.value = 1}).patch<tp::Late>([](tp::Late& late) { late.value = 4; }).getEntity();
    EXPECT_EQ(world.getComponents().getRegistry().find<tp::Late>(), nullptr);
    commands.flush();
    EXPECT_EQ(world.getComponents().get<tp::Late>(entity)->value, 4);
}

TEST(CommandsPatch, ByIdEditsTheBytes) {
    ecs::World world;
    const ecs::ComponentId id = world.getComponents().registerType<tp::Health>();
    const ecs::Entity entity = world.spawn();
    static_cast<void>(world.getComponents().add(entity, tp::Health{.value = 1}));
    ecs::Commands commands{world};
    EXPECT_TRUE(commands.patch(entity, id, [](std::span<std::byte> value) {
        const std::int32_t points = 9;
        ASSERT_EQ(value.size(), sizeof(points));
        std::memcpy(value.data(), &points, sizeof(points));
    }));
    commands.flush();
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 9);
}

TEST(CommandsPatch, ByIdRefusesAnUnknownIdTheHierarchyAndAnEmptyEdit) {
    ecs::World world;
    const ecs::ComponentId health = world.getComponents().registerType<tp::Health>();
    const ecs::Entity parent = world.spawn();
    const ecs::Entity child = world.spawn();
    ASSERT_TRUE(world.setParent(child, parent));
    const ecs::ComponentId parentId = world.getComponents().getRegistry().find<ecs::Parent>()->type.id;
    const ecs::ComponentId childrenId = world.getComponents().getRegistry().find<ecs::Children>()->type.id;
    const auto edit = [](std::span<std::byte> /*value*/) {};
    ecs::Commands commands{world};
    EXPECT_FALSE(commands.patch(child, ecs::ComponentId{9999}, edit));
    EXPECT_FALSE(commands.patch(child, parentId, edit));
    EXPECT_FALSE(commands.patch(parent, childrenId, edit));
    EXPECT_FALSE(commands.patch(child, health, nullptr));
    EXPECT_TRUE(commands.isEmpty());
}

TEST(CommandsPatch, ClearDropsTheEditsWithoutRunningThem) {
    ecs::World world;
    const ecs::Entity entity = world.spawn();
    static_cast<void>(world.getComponents().add(entity, tp::Health{.value = 1}));
    const auto token = std::make_shared<int>(0);
    {
        ecs::Commands commands{world};
        commands.patch<tp::Health>(entity, [token](tp::Health& health) { health.value = *token; });
        EXPECT_EQ(commands.getCount(), 1U);
        EXPECT_EQ(token.use_count(), 2);  // held by the buffer
        commands.clear();
        EXPECT_EQ(token.use_count(), 1);
        commands.flush();
    }
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 1);
}

TEST(CommandsPatch, AMovedBufferKeepsItsEdits) {
    ecs::World world;
    const ecs::Entity entity = world.spawn();
    static_cast<void>(world.getComponents().add(entity, tp::Health{.value = 1}));
    ecs::Commands first{world};
    first.patch<tp::Health>(entity, [](tp::Health& health) { health.value = 6; });
    ecs::Commands second{std::move(first)};
    EXPECT_TRUE(first.isEmpty());  // NOLINT(bugprone-use-after-move): a moved-from buffer is empty
    first.flush();
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 1);
    second.flush();
    EXPECT_EQ(world.getComponents().get<tp::Health>(entity)->value, 6);
}
