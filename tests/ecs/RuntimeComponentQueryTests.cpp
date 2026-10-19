/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeComponentQueryTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::runtime_query {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.runtimeQuery.Position", x, y);
}  // namespace rtype::test::runtime_query

namespace {
using rtype::ecs::ComponentId;
using rtype::ecs::DynamicQuery;
using rtype::ecs::Entity;
using rtype::ecs::QueryBuilder;
using rtype::ecs::QueryMatch;
using rtype::test::runtime_query::Position;

constexpr std::size_t kStrength = 0;  ///< Offset of the shield's `strength` field.
constexpr std::size_t kRegen = 4;     ///< Offset of the shield's `regen` field.

/// @return A component described at run time, as the Luau bridge registers one: no C++ type behind it.
rtype::ecs::ComponentInfo makeShield() {
    return rtype::ecs::ComponentInfo{
        .type = {.name = "test.runtimeQuery.Shield",
                 .size = 8,
                 .alignment = 4,
                 .fields = {{.name = "strength", .type = rtype::ecs::FieldType::kF32, .offset = kStrength},
                            {.name = "regen", .type = rtype::ecs::FieldType::kF32, .offset = kRegen}}}};
}

/// @return The bytes of a shield value.
std::vector<std::byte> shieldBytes(float strength, float regen) {
    std::vector<std::byte> bytes(sizeof(float) * 2);
    std::memcpy(bytes.data() + kStrength, &strength, sizeof(float));
    std::memcpy(bytes.data() + kRegen, &regen, sizeof(float));
    return bytes;
}

/// @return The float stored at `offset` in `bytes`.
float floatAt(std::span<const std::byte> bytes, std::size_t offset) {
    float value = 0.F;
    std::memcpy(&value, bytes.subspan(offset, sizeof(float)).data(), sizeof(float));
    return value;
}

/// @return The entities `query` matches since `lastRun`, by index.
std::vector<Entity> entitiesOf(const DynamicQuery& query, rtype::ecs::Tick lastRun = 0) {
    std::vector<Entity> entities;
    for (const QueryMatch& match : query.iter(lastRun)) {
        entities.push_back(match.getEntity());
    }
    std::ranges::sort(entities, {}, &Entity::getIndex);
    return entities;
}

/// @brief A world with a runtime component (Shield) and a C++ one (Position): one entity has both, one each.
class RuntimeComponentQuery : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(components.add(both, shield, shieldBytes(10.F, 1.F)));
        ASSERT_TRUE(components.add(onlyShield, shield, shieldBytes(20.F, 2.F)));
        ASSERT_TRUE(components.add(both, Position{.x = 1.F, .y = 2.F}));
        ASSERT_TRUE(components.add(onlyPosition, Position{.x = 3.F, .y = 4.F}));
    }

    rtype::ecs::World world;                                     ///< The world under test.
    rtype::ecs::Components& components{world.getComponents()};   ///< Its components.
    ComponentId shield{*components.registerType(makeShield())};  ///< The runtime component.
    ComponentId position{components.registerType<Position>()};   ///< The C++ component.
    Entity both{world.spawn()};                                  ///< Has a shield and a position.
    Entity onlyShield{world.spawn()};                            ///< Has a shield only.
    Entity onlyPosition{world.spawn()};                          ///< Has a position only.
};
}  // namespace

TEST_F(RuntimeComponentQuery, ReadsItsValues) {
    const rtype::ecs::Result<DynamicQuery> query = QueryBuilder{}.read(shield).build(components);
    ASSERT_TRUE(query);
    float total = 0.F;
    for (const QueryMatch& match : query->iter(0)) {
        total += floatAt(match.get(0), kStrength);
    }
    EXPECT_EQ(total, 30.F);
    EXPECT_EQ(entitiesOf(*query), (std::vector<Entity>{both, onlyShield}));
}

TEST_F(RuntimeComponentQuery, WritesItNextToACppComponent) {
    const rtype::ecs::Result<DynamicQuery> query = QueryBuilder{}.read(position).write(shield).build(components);
    ASSERT_TRUE(query);
    for (QueryMatch& match : query->iter(0)) {
        const float strength = floatAt(match.get(1), kStrength) + floatAt(match.get(0), 0);
        std::memcpy(match.getMut(1).subspan(kStrength).data(), &strength, sizeof(float));
    }
    EXPECT_EQ(floatAt(components.get(both, shield), kStrength), 11.F);        // 10 + its position's x
    EXPECT_EQ(floatAt(components.get(onlyShield, shield), kStrength), 20.F);  // not matched: untouched
}

TEST_F(RuntimeComponentQuery, FiltersOnIt) {
    const rtype::ecs::Result<DynamicQuery> shieldOnly = QueryBuilder{}.with(shield).without(position).build(components);
    ASSERT_TRUE(shieldOnly);
    EXPECT_EQ(entitiesOf(*shieldOnly), (std::vector<Entity>{onlyShield}));

    const rtype::ecs::Result<DynamicQuery> unshielded = QueryBuilder{}.read(position).without(shield).build(components);
    ASSERT_TRUE(unshielded);
    EXPECT_EQ(entitiesOf(*unshielded), (std::vector<Entity>{onlyPosition}));
}

TEST_F(RuntimeComponentQuery, ReadsItAsAnOptionalTerm) {
    const rtype::ecs::Result<DynamicQuery> query = QueryBuilder{}.read(position).optionalRead(shield).build(components);
    ASSERT_TRUE(query);
    for (const QueryMatch& match : query->iter(0)) {
        EXPECT_EQ(match.get(1).empty(), match.getEntity() == onlyPosition);  // no shield: an empty value
    }
}

TEST_F(RuntimeComponentQuery, TracksItsChanges) {
    world.advanceChangeTick(5);
    const rtype::ecs::Result<DynamicQuery> writer = QueryBuilder{}.write(shield).with(position).build(components);
    ASSERT_TRUE(writer);
    for (QueryMatch& match : writer->iter(0)) {
        static_cast<void>(match.getMut(0));  // write access stamps the tick
    }
    const rtype::ecs::Result<DynamicQuery> changed = QueryBuilder{}.read(shield).changed(shield).build(components);
    ASSERT_TRUE(changed);
    EXPECT_EQ(entitiesOf(*changed, 3), (std::vector<Entity>{both}));  // written at 5, after tick 3

    const rtype::ecs::Result<DynamicQuery> added = QueryBuilder{}.read(shield).added(shield).build(components);
    ASSERT_TRUE(added);
    EXPECT_TRUE(entitiesOf(*added, 3).empty());  // added at 0
}
