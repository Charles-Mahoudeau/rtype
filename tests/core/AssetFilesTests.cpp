/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetFilesTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <system_error>

#include "core/asset/AssetFiles.hpp"

using rtype::core::asset::isInsideDirectory;

namespace {
/// @brief A folder with an `inner` and an `outer` side, removed when the test ends.
class Sandbox : public ::testing::Test {
  protected:
    void SetUp() override {
        _base = std::filesystem::temp_directory_path() / "rtype-inside-directory";
        std::filesystem::remove_all(_base);
        _root = _base / "root";
        _outside = _base / "outside";
        std::filesystem::create_directories(_root / "inner");
        std::filesystem::create_directories(_outside);
        std::ofstream{_root / "inner" / "a.txt"} << "a";
        std::ofstream{_outside / "b.txt"} << "b";
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_base, error);
    }

    [[nodiscard]] bool link(const std::filesystem::path& where, const std::filesystem::path& target) const {
        std::error_code error;
        std::filesystem::create_symlink(target, where, error);
        return !error;
    }

    std::filesystem::path _base;
    std::filesystem::path _root;
    std::filesystem::path _outside;
};
}  // namespace

TEST_F(Sandbox, APathUnderTheDirectoryIsInside) {
    EXPECT_TRUE(isInsideDirectory(_root, _root / "inner" / "a.txt"));
    EXPECT_TRUE(isInsideDirectory(_root, _root / "missing" / "file.txt"));
    EXPECT_TRUE(isInsideDirectory(_root, _root / "inner" / ".." / "inner" / "a.txt"));
}

TEST_F(Sandbox, TheDirectoryItselfIsInside) {
    EXPECT_TRUE(isInsideDirectory(_root, _root));
    EXPECT_TRUE(isInsideDirectory(_root / "", _root));
}

TEST_F(Sandbox, ADotDotThatLeavesTheDirectoryIsOutside) {
    EXPECT_FALSE(isInsideDirectory(_root, _root / ".." / "outside" / "b.txt"));
    EXPECT_FALSE(isInsideDirectory(_root, _root / "inner" / ".." / ".." / "outside"));
}

TEST_F(Sandbox, ASiblingWithTheSamePrefixIsOutside) {
    std::filesystem::create_directories(_base / "root-other");
    EXPECT_FALSE(isInsideDirectory(_root, _base / "root-other" / "a.txt"));
}

TEST_F(Sandbox, AnAbsolutePathElsewhereIsOutside) { EXPECT_FALSE(isInsideDirectory(_root, _outside / "b.txt")); }

TEST_F(Sandbox, ALinkPointingOutIsOutside) {
    if (!link(_root / "out", _outside)) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    EXPECT_FALSE(isInsideDirectory(_root, _root / "out" / "b.txt"));
    EXPECT_FALSE(isInsideDirectory(_root, _root / "out"));
}

TEST_F(Sandbox, ALinkStayingInsideIsInside) {
    if (!link(_root / "alias", _root / "inner")) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    EXPECT_TRUE(isInsideDirectory(_root, _root / "alias" / "a.txt"));
}
