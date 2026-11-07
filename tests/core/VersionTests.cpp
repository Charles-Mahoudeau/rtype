/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VersionTests
*/

#include <gtest/gtest.h>

#include <string>

#include "core/version/Version.hpp"

namespace {

using rtype::core::version::describe;
using rtype::core::version::VersionInfo;

}  // namespace

TEST(Version, DescribesTheBuildOnFourLines) {
    const VersionInfo info{
        .version = "0.1.0", .commit = "772b71bd", .toolchain = "clang 21.1.0", .libraryAbiVersion = 3};

    EXPECT_EQ(describe(info), "aylo-engine 0.1.0\ncommit: 772b71bd\ntoolchain: clang 21.1.0\nlibrary ABI: 3\n");
}

TEST(Version, KeepsTheNightlyStampInTheFirstLine) {
    const VersionInfo info{
        .version = "0.1.0-nightly.772b71bd", .commit = "772b71bd", .toolchain = "msvc 1944", .libraryAbiVersion = 1};

    EXPECT_TRUE(describe(info).starts_with("aylo-engine 0.1.0-nightly.772b71bd\n"));
}

TEST(Version, NamesAnUnknownCommit) {
    const VersionInfo info{.version = "0.1.0", .commit = "", .toolchain = "gcc 14", .libraryAbiVersion = 1};

    EXPECT_NE(describe(info).find("commit: unknown\n"), std::string::npos);
}
