/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldListTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::ecs::Entity;
using rtype::ecs::FieldInfo;
using rtype::ecs::FieldType;
using rtype::ecs::TypeTraits;
using rtype::ecs::World;

namespace {
/// @brief A component with a list of numbers and a list of entities around a plain field.
struct Inventory {
    std::vector<std::uint32_t> items;
    std::int32_t gold{0};
    std::vector<Entity> owners;
};
RTYPE_ECS_COMPONENT(Inventory, "test.list.Inventory", items, gold, owners);

/// @brief The same fields without the lists: its layout hash must not depend on a list it does not have.
struct Purse {
    std::int32_t gold{0};
};
RTYPE_ECS_COMPONENT(Purse, "test.list.Purse", gold);

/// @return The field of @p info called @p name.
const FieldInfo& fieldOf(const rtype::ecs::TypeInfo& info, std::size_t index) { return info.fields.at(index); }
}  // namespace

TEST(FieldList, ReflectsAVectorAsAListOfItsValues) {
    const rtype::ecs::TypeInfo info = TypeTraits<Inventory>::makeTypeInfo();
    ASSERT_EQ(info.fields.size(), 3U);
    EXPECT_EQ(fieldOf(info, 0).type, FieldType::kU32);
    EXPECT_NE(fieldOf(info, 0).list, nullptr);
    EXPECT_EQ(fieldOf(info, 1).list, nullptr);
    EXPECT_EQ(fieldOf(info, 2).type, FieldType::kEntity);
    EXPECT_NE(fieldOf(info, 2).list, nullptr);
}

TEST(FieldList, IsAValidField) {
    World world;
    EXPECT_NO_THROW(static_cast<void>(world.getComponents().registerType<Inventory>()));
}

TEST(FieldList, GivesTheValuesOfAVector) {
    const Inventory inventory{.items = {7, 8, 9}};
    const FieldInfo field = fieldOf(TypeTraits<Inventory>::makeTypeInfo(), 0);
    const std::span<const std::byte> values = field.list->elements(&inventory.items);
    ASSERT_EQ(values.size(), 3 * sizeof(std::uint32_t));
    EXPECT_EQ(*reinterpret_cast<const std::uint32_t*>(values.data() + sizeof(std::uint32_t)), 8U);  // NOLINT
}

TEST(FieldList, ReplacesTheValuesOfAVector) {
    Inventory inventory{.items = {1, 2, 3, 4}};
    const FieldInfo field = fieldOf(TypeTraits<Inventory>::makeTypeInfo(), 0);
    const std::vector<std::uint32_t> values{5, 6};
    field.list->assign(&inventory.items, std::as_bytes(std::span{values}));
    EXPECT_EQ(inventory.items, values);
    field.list->assign(&inventory.items, {});
    EXPECT_TRUE(inventory.items.empty());
}

TEST(FieldList, ErasesEveryValueEqualToOne) {
    Inventory inventory{.items = {1, 2, 1, 3}};
    const FieldInfo field = fieldOf(TypeTraits<Inventory>::makeTypeInfo(), 0);
    const std::uint32_t value = 1;
    field.list->erase(&inventory.items, std::as_bytes(std::span{&value, 1}));
    EXPECT_EQ(inventory.items, (std::vector<std::uint32_t>{2, 3}));
}

TEST(FieldList, ChangesTheLayoutHashOnlyForListFields) {
    EXPECT_NE(TypeTraits<Inventory>::kLayoutHash, TypeTraits<Purse>::kLayoutHash);
    World world;
    static_cast<void>(world.getComponents().registerType<Inventory>());
    EXPECT_EQ(world.getComponents().getRegistry().find<Inventory>()->type.layoutHash,
              TypeTraits<Inventory>::kLayoutHash);  // what Commands recognizes a component by
}

TEST(FieldList, LosesADespawnedEntityAndKeepsTheOthers) {
    World world;
    const Entity holder = world.spawn();
    const Entity gone = world.spawn();
    const Entity kept = world.spawn();
    world.getComponents().add(holder, Inventory{.items = {1}, .gold = 5, .owners = {gone, kept, gone}});
    EXPECT_TRUE(world.despawn(gone));
    const Inventory* inventory = world.getComponents().get<Inventory>(holder);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->owners, std::vector<Entity>{kept});
    EXPECT_EQ(inventory->items, std::vector<std::uint32_t>{1});
    EXPECT_EQ(inventory->gold, 5);
}

TEST(FieldList, MarksTheValueChangedOnlyWhenAnEntityLeft) {
    World world;
    const Entity holder = world.spawn();
    const Entity gone = world.spawn();
    const Entity stranger = world.spawn();
    world.getComponents().add(holder, Inventory{.owners = {gone}});
    const auto before = world.getComponents().getTicks<Inventory>(holder);
    ASSERT_TRUE(before);
    static_cast<void>(world.advanceChangeTick());
    EXPECT_TRUE(world.despawn(stranger));
    EXPECT_EQ(world.getComponents().getTicks<Inventory>(holder)->changed, before->changed);
    EXPECT_TRUE(world.despawn(gone));
    EXPECT_NE(world.getComponents().getTicks<Inventory>(holder)->changed, before->changed);
}

TEST(FieldList, KeepsItsValuesThroughCommands) {
    World world;
    const Entity holder = world.spawn();
    rtype::ecs::Commands commands{world};
    commands.entity(holder).add(Inventory{.items = {3, 4}});
    commands.flush();
    EXPECT_EQ(world.getComponents().get<Inventory>(holder)->items, (std::vector<std::uint32_t>{3, 4}));
}
