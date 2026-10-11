/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeConfigTests
*/

#include <gtest/gtest.h>

#include <utility>

#include "rtype/luau/RuntimeConfig.hpp"

using Libs = rtype::luau::RuntimeConfig::Libs;

TEST(RuntimeConfig, OrCombinesFlags) {
    const Libs combined = Libs::kBase | Libs::kMath;

    EXPECT_EQ(std::to_underlying(combined), std::to_underlying(Libs::kBase) + std::to_underlying(Libs::kMath));
}

TEST(RuntimeConfig, OrWithNoneIsIdentity) { EXPECT_EQ(Libs::kTable | Libs::kNone, Libs::kTable); }

TEST(RuntimeConfig, OrIsIdempotent) { EXPECT_EQ(Libs::kString | Libs::kString, Libs::kString); }

TEST(RuntimeConfig, AndKeepsCommonFlags) {
    const Libs left = Libs::kBase | Libs::kMath;
    const Libs right = Libs::kMath | Libs::kTable;

    EXPECT_EQ(left & right, Libs::kMath);
}

TEST(RuntimeConfig, AndOfDisjointFlagsIsNone) { EXPECT_EQ(Libs::kBase & Libs::kMath, Libs::kNone); }

TEST(RuntimeConfig, StandardContainsEveryLibrary) {
    for (const Libs lib : {Libs::kBase, Libs::kMath, Libs::kTable, Libs::kString, Libs::kCoroutine, Libs::kBit32,
                           Libs::kUtf8, Libs::kOs, Libs::kDebug, Libs::kBuffer, Libs::kVector}) {
        EXPECT_EQ(Libs::kStandard & lib, lib);
    }
}

TEST(RuntimeConfig, FlagsAreDistinctBits) {
    Libs seen = Libs::kNone;

    for (const Libs lib : {Libs::kBase, Libs::kMath, Libs::kTable, Libs::kString, Libs::kCoroutine, Libs::kBit32,
                           Libs::kUtf8, Libs::kOs, Libs::kDebug, Libs::kBuffer, Libs::kVector}) {
        EXPECT_EQ(seen & lib, Libs::kNone);
        seen = seen | lib;
    }
    EXPECT_EQ(seen, Libs::kStandard);
}
