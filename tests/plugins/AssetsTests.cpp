/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetsTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "FakeBackend.hpp"
#include "plugins/backend/Assets.hpp"
#include "plugins/backend/TextureAssets.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
constexpr rtype::plugins::backend::graphics::TextureId kShip{.index = 0, .generation = 1};
constexpr rtype::plugins::backend::graphics::TextureId kEnemy{.index = 1, .generation = 1};
}  // namespace

static_assert(rtype::ecs::Resource<rtype::plugins::backend::TextureAssets>);

TEST(Assets, StartsEmpty) {
    const rtype::plugins::backend::TextureAssets assets;
    EXPECT_EQ(assets.getCount(), 0U);
    EXPECT_FALSE(assets.find("ship.png"));
    EXPECT_EQ(assets.getInfo(kShip), nullptr);
    EXPECT_FALSE(assets.getName(kShip));
}

TEST(Assets, FindsAnAssetByNameAndId) {
    rtype::plugins::backend::TextureAssets assets;
    EXPECT_TRUE(assets.add("ship.png", kShip, {.size = {32, 16}}));
    EXPECT_EQ(assets.getCount(), 1U);
    EXPECT_EQ(assets.find("ship.png"), kShip);
    ASSERT_NE(assets.getInfo(kShip), nullptr);
    EXPECT_EQ(assets.getInfo(kShip)->size, glm::uvec2(32, 16));
    EXPECT_EQ(assets.getName(kShip), std::optional<std::string_view>{"ship.png"});
}

TEST(Assets, RefusesATakenNameOrId) {
    rtype::plugins::backend::TextureAssets assets;
    assets.add("ship.png", kShip, {.size = {32, 16}});
    EXPECT_FALSE(assets.add("ship.png", kEnemy, {.size = {8, 8}}));
    EXPECT_FALSE(assets.add("other.png", kShip, {.size = {8, 8}}));
    EXPECT_EQ(assets.getCount(), 1U);
    EXPECT_EQ(assets.getInfo(kShip)->size, glm::uvec2(32, 16));
}

TEST(Assets, AStaleIdIsNotFound) {
    rtype::plugins::backend::TextureAssets assets;
    assets.add("ship.png", kShip, {.size = {32, 16}});
    EXPECT_EQ(assets.getInfo({.index = 0, .generation = 2}), nullptr);
}

TEST(Assets, UnloadDestroysTheAssetAndForgetsIt) {
    auto state = std::make_shared<rtype::test::plugins::FakeBackendState>();
    rtype::test::plugins::FakeRenderer renderer{state};
    rtype::plugins::backend::TextureAssets assets;
    assets.add("ship.png", kShip, {.size = {32, 16}});
    assets.add("enemy.png", kEnemy, {.size = {16, 16}});
    EXPECT_TRUE(assets.unload(renderer, "ship.png"));
    ASSERT_EQ(state->destroyedTextures.size(), 1U);
    EXPECT_EQ(state->destroyedTextures[0], kShip);
    EXPECT_FALSE(assets.find("ship.png"));
    EXPECT_EQ(assets.find("enemy.png"), kEnemy);
    EXPECT_FALSE(assets.unload(renderer, "ship.png"));
    EXPECT_EQ(state->destroyedTextures.size(), 1U);
}

TEST(Assets, LivesInTheWorldAsAResource) {
    rtype::ecs::World world;
    rtype::plugins::backend::TextureAssets assets;
    assets.add("ship.png", kShip, {.size = {32, 16}});
    world.getResources().insert(std::move(assets));
    EXPECT_EQ(world.getResources().require<rtype::plugins::backend::TextureAssets>().find("ship.png"), kShip);
    EXPECT_TRUE(world.getResources().getRegistry().findByName("rtype.TextureAssets"));
}
