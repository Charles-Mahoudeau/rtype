/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DrawListTests
*/

#include <gtest/gtest.h>

#include <vector>

#include "render/vulkan/rendering/DrawList.hpp"

using rtype::render::vulkan::rendering::DrawList;
using SpriteRun = DrawList::SpriteRun;
using ParticleStep = DrawList::ParticleStep;

TEST(DrawList, OnlySpritesMakeOneRun) {
    const DrawList list;
    EXPECT_EQ(list.getSteps(3), (std::vector<DrawList::Step>{SpriteRun{.firstBatch = 0, .batchCount = 3}}));
}

TEST(DrawList, NothingMakesNoStep) {
    const DrawList list;
    EXPECT_TRUE(list.getSteps(0).empty());
}

TEST(DrawList, ParticlesSplitTheSpritesWhereTheyWereDrawn) {
    DrawList list;
    list.addParticles(2, 0);
    list.addParticles(3, 1);
    EXPECT_EQ(list.getSteps(5),
              (std::vector<DrawList::Step>{SpriteRun{.firstBatch = 0, .batchCount = 2}, ParticleStep{.draw = 0},
                                           SpriteRun{.firstBatch = 2, .batchCount = 1}, ParticleStep{.draw = 1},
                                           SpriteRun{.firstBatch = 3, .batchCount = 2}}));
}

TEST(DrawList, ParticlesBeforeAnySpriteOrAfterEverySprite) {
    DrawList list;
    list.addParticles(0, 0);
    list.addParticles(2, 1);
    EXPECT_EQ(list.getSteps(2),
              (std::vector<DrawList::Step>{ParticleStep{.draw = 0}, SpriteRun{.firstBatch = 0, .batchCount = 2},
                                           ParticleStep{.draw = 1}}));
}

TEST(DrawList, ParticlesInARowMakeNoEmptyRun) {
    DrawList list;
    list.addParticles(1, 0);
    list.addParticles(1, 1);
    EXPECT_EQ(list.getSteps(1), (std::vector<DrawList::Step>{SpriteRun{.firstBatch = 0, .batchCount = 1},
                                                             ParticleStep{.draw = 0}, ParticleStep{.draw = 1}}));
}

TEST(DrawList, ClearForgetsTheParticles) {
    DrawList list;
    list.addParticles(1, 0);
    list.clear();
    EXPECT_EQ(list.getSteps(2), (std::vector<DrawList::Step>{SpriteRun{.firstBatch = 0, .batchCount = 2}}));
}
