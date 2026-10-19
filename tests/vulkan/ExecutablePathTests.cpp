/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ExecutablePathTests
*/

#include <gtest/gtest.h>

#include <filesystem>

#include "plugins/backend/system/ExecutablePath.hpp"

TEST(ExecutablePath, IsThisTestBinary) {
    const std::filesystem::path path = rtype::plugins::backend::system::getExecutablePath();
    EXPECT_TRUE(path.is_absolute());
    EXPECT_TRUE(std::filesystem::is_regular_file(path));
    EXPECT_EQ(path.stem(), "vulkan-tests");  // .exe on Windows.
}

TEST(ExecutablePath, DirectoryContainsIt) {
    const std::filesystem::path directory = rtype::plugins::backend::system::getExecutableDirectory();
    EXPECT_TRUE(std::filesystem::is_directory(directory));
    EXPECT_EQ(directory, rtype::plugins::backend::system::getExecutablePath().parent_path());
}
