/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPathTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/modules/ScriptPath.hpp"

using rtype::scripting::ErrorKind;
using rtype::scripting::modules::ScriptPath;

namespace {
using Segments = std::vector<std::string>;

void expectInvalid(const std::string& text, const std::string& reason) {
    const auto path = ScriptPath::parse(text);
    ASSERT_FALSE(path) << "'" << text << "' was accepted";
    EXPECT_EQ(path.error().kind(), ErrorKind::kInvalidPath);
    EXPECT_NE(std::string{path.error().message()}.find(reason), std::string::npos)
        << "'" << text << "': " << path.error().message();
}
}  // namespace

TEST(ScriptPath, ParsesACurrentFolderPath) {
    const auto path = ScriptPath::parse("./config/player");
    ASSERT_TRUE(path);
    EXPECT_EQ(path->kind, ScriptPath::Kind::kRelative);
    EXPECT_EQ(path->parents, 0U);
    EXPECT_EQ(path->segments, (Segments{"config", "player"}));
}

TEST(ScriptPath, CountsTheLeadingParents) {
    const auto path = ScriptPath::parse("../../shared/util");
    ASSERT_TRUE(path);
    EXPECT_EQ(path->kind, ScriptPath::Kind::kRelative);
    EXPECT_EQ(path->parents, 2U);
    EXPECT_EQ(path->segments, (Segments{"shared", "util"}));
}

TEST(ScriptPath, ParsesAnAliasWithAPath) {
    const auto path = ScriptPath::parse("@scripts/a/b");
    ASSERT_TRUE(path);
    EXPECT_EQ(path->kind, ScriptPath::Kind::kAlias);
    EXPECT_EQ(path->alias, "scripts");
    EXPECT_EQ(path->segments, (Segments{"a", "b"}));
}

TEST(ScriptPath, ParsesABareAlias) {
    const auto path = ScriptPath::parse("@self");
    ASSERT_TRUE(path);
    EXPECT_EQ(path->kind, ScriptPath::Kind::kAlias);
    EXPECT_EQ(path->alias, "self");
    EXPECT_TRUE(path->segments.empty());
}

TEST(ScriptPath, LowercasesTheAliasButNotTheSegments) {
    const auto path = ScriptPath::parse("@Shared/Util");
    ASSERT_TRUE(path);
    EXPECT_EQ(path->alias, "shared");
    EXPECT_EQ(path->segments, (Segments{"Util"}));
}

TEST(ScriptPath, RejectsAnEmptyString) { expectInvalid("", "empty"); }

TEST(ScriptPath, RejectsAPathWithoutPrefix) {
    expectInvalid("config", "must start with");
    expectInvalid("/config", "must start with");
    expectInvalid(".", "must start with");
    expectInvalid("..", "must start with");
    expectInvalid(".config", "must start with");
}

TEST(ScriptPath, RejectsABackslash) {
    expectInvalid(".\\config", "separator");
    expectInvalid("./a\\b", "separator");
}

TEST(ScriptPath, RejectsAnEmptySegment) {
    expectInvalid("./a//b", "empty segment");
    expectInvalid("@scripts//a", "empty segment");
}

TEST(ScriptPath, RejectsADotSegmentAfterThePrefix) {
    expectInvalid("./a/./b", "only allowed at the start");
    expectInvalid("./a/../b", "only allowed at the start");
    expectInvalid("./../a", "only allowed at the start");
    expectInvalid(".././a", "only allowed at the start");
    expectInvalid("../a/../b", "only allowed at the start");
    expectInvalid("@scripts/../a", "only allowed at the start");
}

TEST(ScriptPath, RejectsATrailingSlash) {
    expectInvalid("./a/", "end with '/'");
    expectInvalid("./", "end with '/'");
    expectInvalid("../", "end with '/'");
    expectInvalid("@scripts/", "end with '/'");
}

TEST(ScriptPath, RejectsAnExtension) {
    expectInvalid("./a.luau", "remove the extension");
    expectInvalid("./a.lua", "remove the extension");
    expectInvalid("@scripts/dir/a.LUAU", "remove the extension");
}

TEST(ScriptPath, AcceptsADotInsideAName) {
    EXPECT_TRUE(ScriptPath::parse("./a.b/c.d"));
    EXPECT_TRUE(ScriptPath::parse("./.hidden"));
}

TEST(ScriptPath, RejectsABadAliasName) {
    expectInvalid("@", "alias name");
    expectInvalid("@/a", "alias name");
    expectInvalid("@a b/c", "alias name");
    expectInvalid("@..", "alias name");
}

TEST(ScriptPath, ChecksAliasNames) {
    EXPECT_TRUE(ScriptPath::isValidAliasName("shared"));
    EXPECT_TRUE(ScriptPath::isValidAliasName("My-lib_2.x"));
    EXPECT_FALSE(ScriptPath::isValidAliasName(""));
    EXPECT_FALSE(ScriptPath::isValidAliasName("."));
    EXPECT_FALSE(ScriptPath::isValidAliasName("a/b"));
    EXPECT_FALSE(ScriptPath::isValidAliasName("a b"));
}
