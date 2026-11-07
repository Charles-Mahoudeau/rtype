/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorWorldViewTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "editor/gui/GuiMesh.hpp"
#include "editor/panels/WorldView.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
struct Health {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Health, "game.Health", value);

struct Armor {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Armor, "game.Armor", value);

struct Poison {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Poison, "other.Poison", value);

struct Burning {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Burning, "Burning", value);
}  // namespace

TEST(EditorWorldView, ShortensANameToWhatFollowsItsNamespace) {
    EXPECT_EQ(rtype::editor::panels::shortName("rtype.Transform"), "Transform");
    EXPECT_EQ(rtype::editor::panels::shortName("a.b.Deep"), "Deep");
    EXPECT_EQ(rtype::editor::panels::shortName("Plain"), "Plain");
    EXPECT_EQ(rtype::editor::panels::shortName(""), "");
}

TEST(EditorWorldView, ListsNothingForAnEmptyWorld) {
    rtype::ecs::World world;
    EXPECT_TRUE(rtype::editor::panels::collectEntities(world, "").empty());
}

TEST(EditorWorldView, ListsEachEntityOnceByIndex) {
    rtype::ecs::World world;
    const rtype::ecs::Entity first = world.spawn(Health{.value = 1}, Armor{.value = 2});
    const rtype::ecs::Entity second = world.spawn(Armor{.value = 3});
    const rtype::ecs::Entity third = world.spawn(Poison{.value = 4});
    const std::vector<rtype::ecs::Entity> entities = rtype::editor::panels::collectEntities(world, "");
    ASSERT_EQ(entities.size(), 3U);
    EXPECT_EQ(entities[0], first);
    EXPECT_EQ(entities[1], second);
    EXPECT_EQ(entities[2], third);
}

TEST(EditorWorldView, DoesNotListAnEntityWithoutAComponent) {
    rtype::ecs::World world;
    static_cast<void>(world.spawn());
    const rtype::ecs::Entity listed = world.spawn(Health{});
    const std::vector<rtype::ecs::Entity> entities = rtype::editor::panels::collectEntities(world, "");
    ASSERT_EQ(entities.size(), 1U);
    EXPECT_EQ(entities[0], listed);
}

TEST(EditorWorldView, KeepsTheEntitiesWithAComponentWhoseNameHoldsTheFilter) {
    rtype::ecs::World world;
    const rtype::ecs::Entity health = world.spawn(Health{});
    const rtype::ecs::Entity both = world.spawn(Armor{}, Poison{});
    static_cast<void>(world.spawn(Burning{}));
    const std::vector<rtype::ecs::Entity> entities = rtype::editor::panels::collectEntities(world, "game.");
    ASSERT_EQ(entities.size(), 2U);
    EXPECT_EQ(entities[0], health);
    EXPECT_EQ(entities[1], both);
}

TEST(EditorWorldView, IgnoresTheCaseOfTheFilter) {
    rtype::ecs::World world;
    const rtype::ecs::Entity entity = world.spawn(Health{});
    const std::vector<rtype::ecs::Entity> entities = rtype::editor::panels::collectEntities(world, "hEaLtH");
    ASSERT_EQ(entities.size(), 1U);
    EXPECT_EQ(entities[0], entity);
}

TEST(EditorWorldView, ListsNothingWhenNoComponentMatchesTheFilter) {
    rtype::ecs::World world;
    static_cast<void>(world.spawn(Health{}));
    EXPECT_TRUE(rtype::editor::panels::collectEntities(world, "nothing").empty());
}

TEST(EditorWorldView, DescribesAnEntityByItsIndexAndItsComponents) {
    rtype::ecs::World world;
    const rtype::ecs::Entity entity = world.spawn(Health{}, Armor{});
    const std::string line = rtype::editor::panels::describeEntity(world, entity);
    EXPECT_EQ(line, std::format("#{}  Health, Armor", entity.getIndex()));
}

TEST(EditorWorldView, DescribesOnlyTheFirstComponentsOfABigEntity) {
    rtype::ecs::World world;
    const rtype::ecs::Entity entity = world.spawn(Health{}, Armor{}, Poison{}, Burning{});
    const std::string line = rtype::editor::panels::describeEntity(world, entity);
    EXPECT_EQ(line, std::format("#{}  Health, Armor, Poison, ...", entity.getIndex()));
}

TEST(EditorGuiMesh, KeepsATextureIdThroughImGui) {
    const rtype::contract::renderer::TextureId texture{.index = 7, .generation = 3};
    const std::uint64_t packed = rtype::editor::gui::toImGuiTexture(texture);
    EXPECT_EQ(rtype::editor::gui::fromImGuiTexture(packed), texture);
}

TEST(EditorGuiMesh, KeepsTheLargestTextureIdThroughImGui) {
    const rtype::contract::renderer::TextureId texture{.index = 0xFFFFFFFFU, .generation = 0xFFFFFFFFU};
    EXPECT_EQ(rtype::editor::gui::fromImGuiTexture(rtype::editor::gui::toImGuiTexture(texture)), texture);
}

TEST(EditorGuiMesh, GivesNoTextureAnIdImGuiReadsAsNone) {
    EXPECT_EQ(rtype::editor::gui::toImGuiTexture(rtype::contract::renderer::TextureId{}), 0U);
    EXPECT_EQ(rtype::editor::gui::fromImGuiTexture(0), rtype::contract::renderer::TextureId{});
}
