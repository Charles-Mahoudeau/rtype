/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTraitsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::test {
/// @brief A math type of the game, reflected through a FieldTypeOf specialization.
struct Vec2 {
    float x;
    float y;
};
}  // namespace rtype::test

template <>
struct rtype::ecs::FieldTypeOf<rtype::test::Vec2>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kVec2> {};

namespace rtype::test {
struct Health {
    std::int32_t value;
    std::int32_t max;
};
RTYPE_ECS_COMPONENT(Health, "rtype.Health", value, max);

struct Path {
    std::array<Vec2, 4> points;
    std::array<float, 3> speeds;
    std::uint8_t flags;
};
RTYPE_ECS_COMPONENT(Path, "test.Path", points, speeds, flags);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "rtype.Dead");

struct Scaled {
    float scale{1.0F};
};
RTYPE_ECS_COMPONENT(Scaled, "test.Scaled", scale);

struct Inventory {
    std::vector<std::int32_t> items;
    std::int32_t gold{5};
};
RTYPE_ECS_COMPONENT(Inventory, "test.Inventory");
}  // namespace rtype::test

struct GlobalPosition {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(GlobalPosition, "test.GlobalPosition", x, y);

static_assert(rtype::ecs::Component<rtype::test::Health>);
static_assert(rtype::ecs::Component<GlobalPosition>);
static_assert(!rtype::ecs::Component<int>);
static_assert(rtype::ecs::ComponentTraits<rtype::test::Health>::kName == "rtype.Health");
static_assert(rtype::ecs::ComponentTraits<rtype::test::Health>::kLayoutHash != 0, "known at compile time");

namespace {
/// @brief Raw storage for one T, as a pool would hold it.
template <typename T>
struct Storage {
    alignas(T) std::array<std::byte, sizeof(T)> bytes;

    void* raw() { return bytes.data(); }
    T& get() { return *std::launder(reinterpret_cast<T*>(bytes.data())); }
};
}  // namespace

TEST(ComponentTraits, RecordsNameAndHash) {
    using Traits = rtype::ecs::ComponentTraits<rtype::test::Health>;
    EXPECT_EQ(Traits::kName, "rtype.Health");
    EXPECT_EQ(Traits::kHash, rtype::ecs::hashName("rtype.Health"));
}

TEST(ComponentTraits, WorksInsideANamespaceAndAtGlobalScope) {
    EXPECT_EQ(rtype::ecs::ComponentTraits<rtype::test::Health>::kFields.size(), 2U);
    EXPECT_EQ(rtype::ecs::ComponentTraits<GlobalPosition>::kName, "test.GlobalPosition");
    EXPECT_EQ(rtype::ecs::ComponentTraits<GlobalPosition>::kFields.at(1).name, "y");
}

TEST(ComponentTraits, FieldsMatchTheStruct) {
    const auto& fields = rtype::ecs::ComponentTraits<rtype::test::Health>::kFields;
    EXPECT_EQ(fields.at(0).name, "value");
    EXPECT_EQ(fields.at(0).type, rtype::ecs::FieldType::kI32);
    EXPECT_EQ(fields.at(0).offset, offsetof(rtype::test::Health, value));
    EXPECT_EQ(fields.at(1).name, "max");
    EXPECT_EQ(fields.at(1).offset, offsetof(rtype::test::Health, max));
    EXPECT_EQ(fields.at(1).count, 1U);
}

TEST(ComponentTraits, ReflectsArraysAndMappedTypes) {
    const auto& fields = rtype::ecs::ComponentTraits<rtype::test::Path>::kFields;
    EXPECT_EQ(fields.at(0).name, "points");
    EXPECT_EQ(fields.at(0).type, rtype::ecs::FieldType::kVec2);
    EXPECT_EQ(fields.at(0).count, 4U);
    EXPECT_EQ(fields.at(1).type, rtype::ecs::FieldType::kF32);
    EXPECT_EQ(fields.at(1).count, 3U);
    EXPECT_EQ(fields.at(1).offset, offsetof(rtype::test::Path, speeds));
    EXPECT_EQ(fields.at(2).type, rtype::ecs::FieldType::kU8);
    EXPECT_EQ(fields.at(2).offset, offsetof(rtype::test::Path, flags));
}

TEST(ComponentTraits, AnEmptyStructIsATag) {
    const rtype::ecs::ComponentInfo info = rtype::ecs::ComponentTraits<rtype::test::Dead>::makeInfo();
    EXPECT_EQ(info.size, 0U);
    EXPECT_TRUE(info.fields.empty());
}

TEST(ComponentTraits, MakeInfoDescribesTheComponent) {
    const rtype::ecs::ComponentInfo info = rtype::ecs::ComponentTraits<rtype::test::Path>::makeInfo();
    EXPECT_EQ(info.name, "test.Path");
    EXPECT_EQ(info.size, sizeof(rtype::test::Path));
    EXPECT_EQ(info.alignment, alignof(rtype::test::Path));
    ASSERT_EQ(info.fields.size(), 3U);
    EXPECT_EQ(info.fields.at(0).name, "points");
    EXPECT_EQ(info.fields.at(0).count, 4U);
    EXPECT_EQ(info.id, rtype::ecs::kInvalidComponentId);  // left to the registry
}

TEST(ComponentTraits, EveryTestComponentIsValid) {
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentTraits<rtype::test::Health>::makeInfo()));
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentTraits<rtype::test::Path>::makeInfo()));
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentTraits<rtype::test::Dead>::makeInfo()));
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentTraits<rtype::test::Scaled>::makeInfo()));
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentTraits<rtype::test::Inventory>::makeInfo()));
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentTraits<GlobalPosition>::makeInfo()));
}

TEST(ComponentHooks, AreNullForPlainData) {
    const rtype::ecs::ComponentHooks hooks = rtype::ecs::ComponentTraits<rtype::test::Health>::kHooks;
    EXPECT_EQ(hooks.construct, nullptr);
    EXPECT_EQ(hooks.destruct, nullptr);
    EXPECT_EQ(hooks.move, nullptr);
    EXPECT_EQ(hooks.copy, nullptr);
}

TEST(ComponentHooks, ConstructAppliesDefaultMemberInitializers) {
    const rtype::ecs::ComponentHooks hooks = rtype::ecs::ComponentTraits<rtype::test::Scaled>::kHooks;
    ASSERT_NE(hooks.construct, nullptr);
    EXPECT_EQ(hooks.destruct, nullptr);
    EXPECT_EQ(hooks.move, nullptr);
    EXPECT_EQ(hooks.copy, nullptr);

    Storage<rtype::test::Scaled> storage{};
    hooks.construct(storage.raw());
    EXPECT_EQ(storage.get().scale, 1.0F);
}

TEST(ComponentHooks, ManageNonTrivialMembers) {
    const rtype::ecs::ComponentHooks hooks = rtype::ecs::ComponentTraits<rtype::test::Inventory>::kHooks;
    ASSERT_NE(hooks.construct, nullptr);
    ASSERT_NE(hooks.destruct, nullptr);
    ASSERT_NE(hooks.move, nullptr);
    ASSERT_NE(hooks.copy, nullptr);

    Storage<rtype::test::Inventory> original{};
    hooks.construct(original.raw());
    EXPECT_EQ(original.get().gold, 5);
    original.get().items = {1, 2, 3};

    Storage<rtype::test::Inventory> copy{};
    hooks.copy(copy.raw(), original.raw());
    EXPECT_EQ(copy.get().items, (std::vector<std::int32_t>{1, 2, 3}));

    Storage<rtype::test::Inventory> moved{};
    hooks.move(moved.raw(), copy.raw());
    EXPECT_EQ(moved.get().items, (std::vector<std::int32_t>{1, 2, 3}));
    EXPECT_EQ(original.get().items.size(), 3U);

    hooks.destruct(original.raw());
    hooks.destruct(copy.raw());
    hooks.destruct(moved.raw());
}

TEST(ComponentTraits, LayoutHashMatchesTheRunTimeHash) {
    EXPECT_EQ(rtype::ecs::ComponentTraits<rtype::test::Health>::kLayoutHash,
              rtype::ecs::computeLayoutHash(rtype::ecs::ComponentTraits<rtype::test::Health>::makeInfo()));
    EXPECT_EQ(rtype::ecs::ComponentTraits<rtype::test::Path>::kLayoutHash,
              rtype::ecs::computeLayoutHash(rtype::ecs::ComponentTraits<rtype::test::Path>::makeInfo()));
    EXPECT_EQ(rtype::ecs::ComponentTraits<rtype::test::Dead>::kLayoutHash,
              rtype::ecs::computeLayoutHash(rtype::ecs::ComponentTraits<rtype::test::Dead>::makeInfo()));
    EXPECT_EQ(rtype::ecs::ComponentTraits<rtype::test::Inventory>::kLayoutHash,
              rtype::ecs::computeLayoutHash(rtype::ecs::ComponentTraits<rtype::test::Inventory>::makeInfo()));
    EXPECT_EQ(rtype::ecs::ComponentTraits<GlobalPosition>::kLayoutHash,
              rtype::ecs::computeLayoutHash(rtype::ecs::ComponentTraits<GlobalPosition>::makeInfo()));
}

TEST(ComponentTraits, LayoutHashDiffersBetweenComponents) {
    EXPECT_NE(rtype::ecs::ComponentTraits<rtype::test::Health>::kLayoutHash,
              rtype::ecs::ComponentTraits<rtype::test::Path>::kLayoutHash);
    EXPECT_NE(rtype::ecs::ComponentTraits<rtype::test::Scaled>::kLayoutHash,
              rtype::ecs::ComponentTraits<rtype::test::Dead>::kLayoutHash);
}
