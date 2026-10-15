/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeTraitsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::test::type_traits {
struct Health {
    std::int32_t value;
    std::int32_t max;
};
RTYPE_ECS_COMPONENT(Health, "test.type_traits.Health", value, max);

struct Inventory {
    std::vector<std::int32_t> items;
};
RTYPE_ECS_COMPONENT(Inventory, "test.type_traits.Inventory");

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.type_traits.Dead");
}  // namespace rtype::test::type_traits

namespace traits = rtype::test::type_traits;

static_assert(rtype::ecs::Described<traits::Health>);
static_assert(!rtype::ecs::Described<int>);
static_assert(rtype::ecs::TypeTraits<traits::Health>::kKind == rtype::ecs::TypeKind::kComponent);
static_assert(rtype::ecs::ComponentTraits<traits::Health>::kName == "test.type_traits.Health",
              "ComponentTraits inherits what TypeTraits knows");

TEST(TypeTraits, DescribesSizeAlignmentAndCopyability) {
    EXPECT_EQ(rtype::ecs::TypeTraits<traits::Health>::kSize, sizeof(traits::Health));
    EXPECT_EQ(rtype::ecs::TypeTraits<traits::Health>::kAlignment, alignof(traits::Health));
    EXPECT_TRUE(rtype::ecs::TypeTraits<traits::Health>::kCopyable);
    EXPECT_EQ(rtype::ecs::TypeTraits<traits::Dead>::kSize, 0U);  // an empty type is a tag
}

TEST(TypeTraits, MakesTheTypeInfo) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<traits::Health>::makeTypeInfo();
    EXPECT_EQ(info.name, "test.type_traits.Health");
    EXPECT_EQ(info.size, sizeof(traits::Health));
    EXPECT_TRUE(info.copyable);
    ASSERT_EQ(info.fields.size(), 2U);
    EXPECT_EQ(info.fields[1].name, "max");
    EXPECT_EQ(info.fields[1].type, rtype::ecs::FieldType::kI32);
    EXPECT_EQ(rtype::ecs::computeLayoutHash(info), rtype::ecs::TypeTraits<traits::Health>::kLayoutHash);
}

TEST(TypeTraits, AComponentInfoWrapsTheTypeInfo) {
    const rtype::ecs::ComponentInfo component = rtype::ecs::ComponentTraits<traits::Inventory>::makeInfo();
    const rtype::ecs::TypeInfo type = rtype::ecs::TypeTraits<traits::Inventory>::makeTypeInfo();
    EXPECT_EQ(component.type.name, type.name);
    EXPECT_EQ(component.type.size, type.size);
    EXPECT_EQ(rtype::ecs::computeLayoutHash(component.type), rtype::ecs::computeLayoutHash(type));
    EXPECT_NE(component.type.hooks.destruct, nullptr);  // the std::vector member needs destroying
}
