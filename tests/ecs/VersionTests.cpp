/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VersionTests
*/

#include <gtest/gtest.h>

#include <regex>
#include <string>

#include "rtype/ecs/Version.hpp"

TEST(Version, IsNotEmpty) { EXPECT_FALSE(rtype::ecs::getVersion().empty()); }

TEST(Version, IsMajorMinorPatch) {
    const std::string version{rtype::ecs::getVersion()};
    EXPECT_TRUE(std::regex_match(version, std::regex{R"(\d+\.\d+\.\d+)"})) << "version: " << version;
}
