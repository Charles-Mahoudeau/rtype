/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorAssetKindTests
*/

#include <gtest/gtest.h>

#include "editor/panels/AssetKind.hpp"

using rtype::editor::panels::AssetKind;
using rtype::editor::panels::kindOf;

TEST(EditorAssetKind, TellsFilesApartByExtension) {
    EXPECT_EQ(kindOf("level.scene", false), AssetKind::kScene);
    EXPECT_EQ(kindOf("scenes/boss.actor", false), AssetKind::kActor);
    EXPECT_EQ(kindOf("thing.capsule", false), AssetKind::kCapsule);
    EXPECT_EQ(kindOf("main.luau", false), AssetKind::kScript);
    EXPECT_EQ(kindOf("ship.png", false), AssetKind::kImage);
    EXPECT_EQ(kindOf("laser.ogg", false), AssetKind::kAudio);
    EXPECT_EQ(kindOf("ui.ttf", false), AssetKind::kFont);
    EXPECT_EQ(kindOf("map.tmj", false), AssetKind::kData);
    EXPECT_EQ(kindOf("README.md", false), AssetKind::kText);
}

TEST(EditorAssetKind, IgnoresCaseAndFallsBackToOther) {
    EXPECT_EQ(kindOf("SHIP.PNG", false), AssetKind::kImage);
    EXPECT_EQ(kindOf("archive.zip", false), AssetKind::kOther);
    EXPECT_EQ(kindOf("noextension", false), AssetKind::kOther);
    EXPECT_EQ(kindOf(".gitkeep", false), AssetKind::kOther);
}

TEST(EditorAssetKind, AFolderIsAFolderWhateverItsName) { EXPECT_EQ(kindOf("sprites.png", true), AssetKind::kFolder); }

TEST(EditorAssetKind, EveryKindHasAnIconAndALabel) {
    for (int i = 0; i <= static_cast<int>(AssetKind::kOther); ++i) {
        const auto kind = static_cast<AssetKind>(i);
        EXPECT_NE(rtype::editor::panels::iconOf(kind), U'\0');
        EXPECT_FALSE(rtype::editor::panels::labelOf(kind).empty());
    }
    EXPECT_NE(rtype::editor::panels::iconOf(AssetKind::kFolder, true),
              rtype::editor::panels::iconOf(AssetKind::kFolder));
}

TEST(EditorAssetKind, EncodesCodePointsAsUtf8) {
    EXPECT_EQ(rtype::editor::panels::toUtf8(U'A'), "A");
    EXPECT_EQ(rtype::editor::panels::toUtf8(U'é'), "\xC3\xA9");
    EXPECT_EQ(rtype::editor::panels::toUtf8(U''), "\xEF\x81\xBB");
    EXPECT_EQ(rtype::editor::panels::toUtf8(U'\U0001F600'), "\xF0\x9F\x98\x80");
}
