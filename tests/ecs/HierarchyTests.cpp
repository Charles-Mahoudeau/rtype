/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HierarchyTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::ecs::Children;
using rtype::ecs::Entity;
using rtype::ecs::Parent;
using rtype::ecs::World;

static_assert(rtype::ecs::Component<Parent>);
static_assert(rtype::ecs::Component<Children>);

TEST(Parent, ReflectsItsEntity) {
    const auto& fields = rtype::ecs::TypeTraits<Parent>::kFields;
    ASSERT_EQ(fields.size(), 1U);
    EXPECT_EQ(fields.front().type, rtype::ecs::FieldType::kEntity);
}

TEST(Parent, IsClearedWhenItsTargetIsDespawned) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    world.getComponents().add(child, Parent{.entity = parent});
    EXPECT_TRUE(world.despawn(parent));
    ASSERT_NE(world.getComponents().get<Parent>(child), nullptr);
    EXPECT_EQ(world.getComponents().get<Parent>(child)->entity, Entity{});
}

TEST(Children, IsNotReflected) { EXPECT_TRUE(rtype::ecs::TypeTraits<Children>::kFields.empty()); }

TEST(Children, KeepsItsEntitiesWhenThePoolMovesThem) {
    World world;
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    const Entity child = world.spawn();
    world.getComponents().add(first, Children{.entities = {child}});
    world.getComponents().add(second, Children{.entities = {child, child}});
    EXPECT_TRUE(world.despawn(first));  // swap-and-pop: second moves into the hole
    ASSERT_NE(world.getComponents().get<Children>(second), nullptr);
    EXPECT_EQ(world.getComponents().get<Children>(second)->entities, (std::vector<Entity>{child, child}));
}

TEST(Children, KeepsItsEntitiesWhenThePoolGrows) {
    constexpr std::size_t kEntities = 2000;
    World world;
    const Entity child = world.spawn();
    const Entity first = world.spawn();
    world.getComponents().add(first, Children{.entities = {child}});
    for (std::size_t index = 0; index < kEntities; ++index) {
        world.getComponents().add(world.spawn(), Children{.entities = {child, child}});
    }
    ASSERT_NE(world.getComponents().get<Children>(first), nullptr);
    EXPECT_EQ(world.getComponents().get<Children>(first)->entities, (std::vector<Entity>{child}));
}

TEST(Children, FreesItsEntitiesWithItsEntity) {
    World world;
    const Entity entity = world.spawn();
    world.getComponents().add(entity, Children{.entities = {world.spawn()}});
    EXPECT_TRUE(world.despawn(entity));
    EXPECT_EQ(world.getComponents().get<Children>(entity), nullptr);
}

namespace {
/// @return The children of @p entity, empty if it has none.
std::vector<Entity> childrenOf(const World& world, Entity entity) {
    const Children* children = world.getComponents().get<Children>(entity);
    return children != nullptr ? children->entities : std::vector<Entity>{};
}

/// @return The parent of @p entity, the null entity if it has none.
Entity parentOf(const World& world, Entity entity) {
    const Parent* parent = world.getComponents().get<Parent>(entity);
    return parent != nullptr ? parent->entity : Entity{};
}
}  // namespace

TEST(SetParent, LinksAChildAndCreatesChildren) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    EXPECT_EQ(parentOf(world, child), parent);
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{child});
}

TEST(SetParent, KeepsTheOrderChildrenWereAttachedIn) {
    World world;
    const Entity parent = world.spawn();
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    EXPECT_TRUE(world.setParent(first, parent));
    EXPECT_TRUE(world.setParent(second, parent));
    EXPECT_EQ(childrenOf(world, parent), (std::vector<Entity>{first, second}));
}

TEST(SetParent, DoesNotReorderAChildGivenTheSameParent) {
    World world;
    const Entity parent = world.spawn();
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    EXPECT_TRUE(world.setParent(first, parent));
    EXPECT_TRUE(world.setParent(second, parent));
    EXPECT_TRUE(world.setParent(first, parent));
    EXPECT_EQ(childrenOf(world, parent), (std::vector<Entity>{first, second}));
}

TEST(SetParent, MovesAChildFromItsPreviousParent) {
    World world;
    const Entity previous = world.spawn();
    const Entity next = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, previous));
    EXPECT_TRUE(world.setParent(child, next));
    EXPECT_EQ(parentOf(world, child), next);
    EXPECT_FALSE(world.getComponents().has<Children>(previous));
    EXPECT_EQ(childrenOf(world, next), std::vector<Entity>{child});
}

TEST(SetParent, KeepsTheOtherChildrenOfThePreviousParent) {
    World world;
    const Entity previous = world.spawn();
    const Entity next = world.spawn();
    const Entity moved = world.spawn();
    const Entity kept = world.spawn();
    EXPECT_TRUE(world.setParent(moved, previous));
    EXPECT_TRUE(world.setParent(kept, previous));
    EXPECT_TRUE(world.setParent(moved, next));
    EXPECT_EQ(childrenOf(world, previous), std::vector<Entity>{kept});
}

TEST(SetParent, RejectsAnEntityAsItsOwnParent) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_FALSE(world.setParent(entity, entity));
    EXPECT_FALSE(world.getComponents().has<Parent>(entity));
}

TEST(SetParent, RejectsADirectCycle) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    EXPECT_FALSE(world.setParent(parent, child));
    EXPECT_FALSE(world.getComponents().has<Parent>(parent));
    EXPECT_EQ(parentOf(world, child), parent);
}

TEST(SetParent, RejectsACycleThroughDescendants) {
    World world;
    const Entity root = world.spawn();
    const Entity middle = world.spawn();
    const Entity leaf = world.spawn();
    EXPECT_TRUE(world.setParent(middle, root));
    EXPECT_TRUE(world.setParent(leaf, middle));
    EXPECT_FALSE(world.setParent(root, leaf));
    EXPECT_FALSE(world.getComponents().has<Parent>(root));
}

TEST(SetParent, RejectsADeadEntity) {
    World world;
    const Entity alive = world.spawn();
    const Entity dead = world.spawn();
    EXPECT_TRUE(world.despawn(dead));
    EXPECT_FALSE(world.setParent(alive, dead));
    EXPECT_FALSE(world.setParent(dead, alive));
    EXPECT_FALSE(world.getComponents().has<Parent>(alive));
}

TEST(SetParent, KeepsThePreviousParentWhenRejected) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    const Entity leaf = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    EXPECT_TRUE(world.setParent(leaf, child));
    EXPECT_FALSE(world.setParent(child, leaf));
    EXPECT_EQ(parentOf(world, child), parent);
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{child});
}

TEST(RemoveParent, MakesAChildARoot) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    EXPECT_TRUE(world.removeParent(child));
    EXPECT_FALSE(world.getComponents().has<Parent>(child));
    EXPECT_FALSE(world.getComponents().has<Children>(parent));
}

TEST(RemoveParent, KeepsTheOtherChildren) {
    World world;
    const Entity parent = world.spawn();
    const Entity removed = world.spawn();
    const Entity kept = world.spawn();
    EXPECT_TRUE(world.setParent(removed, parent));
    EXPECT_TRUE(world.setParent(kept, parent));
    EXPECT_TRUE(world.removeParent(removed));
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{kept});
}

TEST(RemoveParent, RefusesARootOrADeadEntity) {
    World world;
    const Entity root = world.spawn();
    const Entity dead = world.spawn();
    EXPECT_TRUE(world.despawn(dead));
    EXPECT_FALSE(world.removeParent(root));
    EXPECT_FALSE(world.removeParent(dead));
}

TEST(Despawn, DestroysEveryDescendant) {
    World world;
    const Entity root = world.spawn();
    const Entity middle = world.spawn();
    const Entity sibling = world.spawn();
    const Entity leaf = world.spawn();
    EXPECT_TRUE(world.setParent(middle, root));
    EXPECT_TRUE(world.setParent(sibling, root));
    EXPECT_TRUE(world.setParent(leaf, middle));
    EXPECT_TRUE(world.despawn(root));
    EXPECT_FALSE(world.isAlive(root));
    EXPECT_FALSE(world.isAlive(middle));
    EXPECT_FALSE(world.isAlive(sibling));
    EXPECT_FALSE(world.isAlive(leaf));
}

TEST(Despawn, TakesAChildOutOfItsParent) {
    World world;
    const Entity parent = world.spawn();
    const Entity gone = world.spawn();
    const Entity kept = world.spawn();
    EXPECT_TRUE(world.setParent(gone, parent));
    EXPECT_TRUE(world.setParent(kept, parent));
    EXPECT_TRUE(world.despawn(gone));
    EXPECT_TRUE(world.isAlive(parent));
    EXPECT_TRUE(world.isAlive(kept));
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{kept});
}

TEST(Despawn, RemovesChildrenFromAParentLeftWithout) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    EXPECT_TRUE(world.despawn(child));
    EXPECT_FALSE(world.getComponents().has<Children>(parent));
}

TEST(Despawn, KeepsTheRestOfTheTreeWhenAMiddleEntityGoes) {
    World world;
    const Entity root = world.spawn();
    const Entity middle = world.spawn();
    const Entity sibling = world.spawn();
    const Entity leaf = world.spawn();
    EXPECT_TRUE(world.setParent(middle, root));
    EXPECT_TRUE(world.setParent(sibling, root));
    EXPECT_TRUE(world.setParent(leaf, middle));
    EXPECT_TRUE(world.despawn(middle));
    EXPECT_FALSE(world.isAlive(leaf));
    EXPECT_TRUE(world.isAlive(root));
    EXPECT_TRUE(world.isAlive(sibling));
    EXPECT_EQ(childrenOf(world, root), std::vector<Entity>{sibling});
}

TEST(Despawn, LeavesAnEntityWithoutHierarchyAlone) {
    World world;
    const Entity gone = world.spawn();
    const Entity other = world.spawn();
    EXPECT_TRUE(world.despawn(gone));
    EXPECT_TRUE(world.isAlive(other));
    EXPECT_FALSE(world.despawn(gone));
}

TEST(Despawn, HandlesADeepChain) {
    constexpr std::size_t kDepth = 1000;
    World world;
    const Entity root = world.spawn();
    Entity last = root;
    for (std::size_t depth = 0; depth < kDepth; ++depth) {
        const Entity next = world.spawn();
        EXPECT_TRUE(world.setParent(next, last));
        last = next;
    }
    EXPECT_TRUE(world.despawn(root));
    EXPECT_FALSE(world.isAlive(last));
}

TEST(Despawn, SkipsDeadOrRepeatedEntriesOfChildrenWrittenByHand) {
    World world;
    const Entity parent = world.spawn();
    const Entity dead = world.spawn();
    const Entity repeated = world.spawn();
    EXPECT_TRUE(world.despawn(dead));
    world.getComponents().add(parent, Children{.entities = {dead, repeated, repeated}});
    EXPECT_TRUE(world.despawn(parent));
    EXPECT_FALSE(world.isAlive(parent));
    EXPECT_FALSE(world.isAlive(repeated));
}

TEST(Despawn, CanReuseAnIndexWithoutReachingTheOldChild) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    EXPECT_TRUE(world.despawn(child));
    const Entity reused = world.spawn();
    EXPECT_TRUE(world.despawn(parent));
    EXPECT_TRUE(world.isAlive(reused));
}

TEST(HierarchyCommands, AddingAParentFillsChildren) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    rtype::ecs::Commands commands{world};
    commands.entity(child).add(Parent{.entity = parent});
    commands.flush();
    EXPECT_EQ(parentOf(world, child), parent);
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{child});
}

TEST(HierarchyCommands, RemovingAParentEmptiesChildren) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    rtype::ecs::Commands commands{world};
    commands.entity(child).remove<Parent>();
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Parent>(child));
    EXPECT_FALSE(world.getComponents().has<Children>(parent));
}

TEST(HierarchyCommands, LinksEntitiesSpawnedInTheSameFlushWhateverTheirOrder) {
    World world;
    rtype::ecs::Commands commands{world};
    const Entity child = commands.spawn().getEntity();
    const Entity parent = commands.spawn().getEntity();
    commands.entity(child).add(Parent{.entity = parent});
    commands.flush();
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{child});
}

TEST(HierarchyCommands, MovesAChildBetweenParents) {
    World world;
    const Entity previous = world.spawn();
    const Entity next = world.spawn();
    const Entity child = world.spawn();
    rtype::ecs::Commands commands{world};
    commands.entity(child).add(Parent{.entity = previous});
    commands.entity(child).add(Parent{.entity = next});
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Children>(previous));
    EXPECT_EQ(childrenOf(world, next), std::vector<Entity>{child});
}

TEST(HierarchyCommands, SkipsACycle) {
    World world;
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    rtype::ecs::Commands commands{world};
    commands.entity(parent).add(Parent{.entity = child});
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Parent>(parent));
    EXPECT_EQ(parentOf(world, child), parent);
}

TEST(HierarchyCommands, GoesThroughTheWorldByIdAsTheNetworkDoes) {
    World world;
    const rtype::ecs::ComponentId id = world.getComponents().registerType<Parent>();
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    const Parent value{.entity = parent};
    rtype::ecs::Commands commands{world};
    EXPECT_TRUE(commands.add(child, id, std::as_bytes(std::span{&value, 1})));
    commands.flush();
    EXPECT_EQ(childrenOf(world, parent), std::vector<Entity>{child});
    EXPECT_TRUE(commands.remove(child, id));
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Parent>(child));
    EXPECT_FALSE(world.getComponents().has<Children>(parent));
}

TEST(HierarchyCommands, KeepsTheOptionsParentWasRegisteredWith) {
    World world;
    const rtype::ecs::ComponentId id =
        world.getComponents().registerType<Parent>({.replication = rtype::ecs::ReplicationMode::kEveryChange});
    const Entity parent = world.spawn();
    const Entity child = world.spawn();
    rtype::ecs::Commands commands{world};
    commands.entity(child).add(Parent{.entity = parent});
    commands.flush();
    EXPECT_EQ(world.getComponents().getRegistry().find<Parent>()->type.id, id);
    EXPECT_EQ(parentOf(world, child), parent);
}
