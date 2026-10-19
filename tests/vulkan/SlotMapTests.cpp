/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SlotMapTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/core/SlotMap.hpp"

namespace {

using TextureId = rtype::plugins::backend::graphics::TextureId;
using Map = rtype::render::vulkan::core::SlotMap<TextureId, std::string>;

}  // namespace

TEST(SlotMap, FindsWhatWasInserted) {
    Map map;
    const TextureId first = map.insert("first");
    const TextureId second = map.insert("second");

    EXPECT_NE(first, second);
    EXPECT_EQ(map.getSize(), 2U);
    ASSERT_NE(map.find(first), nullptr);
    EXPECT_EQ(*map.find(first), "first");
    ASSERT_NE(map.find(second), nullptr);
    EXPECT_EQ(*map.find(second), "second");
}

TEST(SlotMap, NeverGivesGenerationZero) {
    Map map;
    EXPECT_NE(map.insert("value").generation, 0U);
    EXPECT_EQ(map.find(TextureId{}), nullptr);
}

TEST(SlotMap, EraseReturnsTheValueOnce) {
    Map map;
    const TextureId key = map.insert("value");

    const std::optional<std::string> erased = map.erase(key);

    ASSERT_TRUE(erased);
    EXPECT_EQ(*erased, "value");
    EXPECT_EQ(map.getSize(), 0U);
    EXPECT_EQ(map.find(key), nullptr);
    EXPECT_FALSE(map.erase(key));
}

TEST(SlotMap, AnOldKeyDoesNotReachTheSlotsNextValue) {
    Map map;
    const TextureId old = map.insert("old");
    static_cast<void>(map.erase(old));

    const TextureId reused = map.insert("new");

    EXPECT_EQ(reused.index, old.index);
    EXPECT_NE(reused.generation, old.generation);
    EXPECT_EQ(map.find(old), nullptr);
    EXPECT_FALSE(map.erase(old));
    ASSERT_NE(map.find(reused), nullptr);
    EXPECT_EQ(*map.find(reused), "new");
}

TEST(SlotMap, IgnoresAnIndexOutOfRange) {
    Map map;
    static_cast<void>(map.insert("value"));

    EXPECT_EQ(map.find(TextureId{.index = 7, .generation = 1}), nullptr);
    EXPECT_FALSE(map.erase(TextureId{.index = 7, .generation = 1}));
    EXPECT_EQ(map.getSize(), 1U);
}

TEST(SlotMap, HoldsMoveOnlyValues) {
    rtype::render::vulkan::core::SlotMap<TextureId, std::unique_ptr<int>> map;
    const TextureId key = map.insert(std::make_unique<int>(42));

    std::optional<std::unique_ptr<int>> erased = map.erase(key);

    ASSERT_TRUE(erased);
    EXPECT_EQ(**erased, 42);
}
