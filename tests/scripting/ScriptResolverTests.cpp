/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptResolverTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

#include "ScriptsDirectory.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"
#include "rtype/scripting/modules/ScriptResolver.hpp"

using rtype::scripting::ErrorKind;
using rtype::scripting::exceptions::ScriptLoadException;
using rtype::scripting::modules::ScriptResolver;
using rtype::tests::ScriptsDirectory;

namespace {
/// @brief Expects the resolution to fail with @p kind and a message containing @p text.
void expectError(const rtype::scripting::Result<rtype::luau::ModuleSource>& result, const ErrorKind kind,
                 const std::string_view text) {
    ASSERT_FALSE(result) << "resolved to " << result->key;
    EXPECT_EQ(result.error().kind(), kind) << result.error().message();
    EXPECT_NE(std::string{result.error().message()}.find(text), std::string::npos) << result.error().message();
}
}  // namespace

TEST(ScriptResolver, ResolvesAFile) {
    const ScriptsDirectory scripts{"rtype-resolver-file"};
    scripts.write("config/player.luau", "return 1");
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("main.luau", "./config/player");
    ASSERT_TRUE(module) << module.error().message();
    EXPECT_EQ(module->key, "config/player.luau");
    EXPECT_EQ(module->chunkName, "scripts/config/player.luau");
    EXPECT_EQ(module->source, "return 1");
}

TEST(ScriptResolver, ResolvesAFolderWithAnInit) {
    const ScriptsDirectory scripts{"rtype-resolver-init"};
    scripts.write("ui/init.luau", "return 'ui'");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("", "@scripts/ui");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "ui/init.luau");
    EXPECT_EQ(module->chunkName, "scripts/ui/init.luau");
}

TEST(ScriptResolver, AFileAndAnInitTogetherAreAmbiguous) {
    const ScriptsDirectory scripts{"rtype-resolver-ambiguous"};
    scripts.write("a.luau", "");
    scripts.write("a/init.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/a"), ErrorKind::kAmbiguousModule, "ambiguous");
}

TEST(ScriptResolver, ANotFoundNamesBothCandidates) {
    const ScriptsDirectory scripts{"rtype-resolver-missing"};
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("main.luau", "./config/player"), ErrorKind::kModuleNotFound,
                "scripts/config/player.luau or scripts/config/player/init.luau");
}

TEST(ScriptResolver, AFolderNamedLikeAFileIsNotAModule) {
    const ScriptsDirectory scripts{"rtype-resolver-folder-file"};
    scripts.write("a.luau/x.txt", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/a"), ErrorKind::kModuleNotFound, "not found");
}

TEST(ScriptResolver, ResolvesFromTheFolderOfTheRequester) {
    const ScriptsDirectory scripts{"rtype-resolver-relative"};
    scripts.write("a/b/main.luau", "");
    scripts.write("a/b/sibling.luau", "");
    scripts.write("a/up.luau", "");
    scripts.write("top.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("a/b/main.luau", "./sibling")->key, "a/b/sibling.luau");
    EXPECT_EQ(resolver("a/b/main.luau", "../up")->key, "a/up.luau");
    EXPECT_EQ(resolver("a/b/main.luau", "../../top")->key, "top.luau");
}

TEST(ScriptResolver, ARootRequireIsRelativeToScripts) {
    const ScriptsDirectory scripts{"rtype-resolver-root-require"};
    scripts.write("server/main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("", "./server/main")->key, "server/main.luau");
}

TEST(ScriptResolver, RefusesToLeaveScripts) {
    const ScriptsDirectory scripts{"rtype-resolver-escape"};
    scripts.write("a/main.luau", "");
    scripts.write("../rtype-resolver-escape-secret.luau", "return 'secret'");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("a/main.luau", "../../rtype-resolver-escape-secret"), ErrorKind::kOutsideScripts,
                "escapes scripts/");
    expectError(resolver("", "../project"), ErrorKind::kOutsideScripts, "escapes scripts/");
    expectError(resolver("main.luau", "../x"), ErrorKind::kOutsideScripts, "escapes scripts/");
    std::filesystem::remove(scripts.path() / ".." / "rtype-resolver-escape-secret.luau");
}

TEST(ScriptResolver, RefusesTheInvalidForms) {
    const ScriptsDirectory scripts{"rtype-resolver-invalid"};
    scripts.write("a.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "a"), ErrorKind::kInvalidPath, "must start with");
    expectError(resolver("", ""), ErrorKind::kInvalidPath, "empty");
    expectError(resolver("", "./a.luau"), ErrorKind::kInvalidPath, "extension");
    expectError(resolver("", ".\\a"), ErrorKind::kInvalidPath, "separator");
    expectError(resolver("", "./a//b"), ErrorKind::kInvalidPath, "empty segment");
    expectError(resolver("", "./a/./b"), ErrorKind::kInvalidPath, "start of a path");
    expectError(resolver("", "./a/"), ErrorKind::kInvalidPath, "end with '/'");
    expectError(resolver("", "@scripts/../a"), ErrorKind::kInvalidPath, "start of a path");
}

TEST(ScriptResolver, TheScriptsAliasIsTheRoot) {
    const ScriptsDirectory scripts{"rtype-resolver-scripts-alias"};
    scripts.write("a/b/main.luau", "");
    scripts.write("shared/util.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("a/b/main.luau", "@scripts/shared/util")->key, "shared/util.luau");
    EXPECT_EQ(resolver("a/b/main.luau", "@SCRIPTS/shared/util")->key, "shared/util.luau");
}

TEST(ScriptResolver, ABareScriptsAliasIsTheRootInit) {
    const ScriptsDirectory scripts{"rtype-resolver-bare-scripts"};
    scripts.write("init.luau", "return 'root'");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("x.luau", "@scripts");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "init.luau");
    EXPECT_EQ(module->chunkName, "scripts/init.luau");

    const ScriptsDirectory empty{"rtype-resolver-bare-scripts-empty"};
    expectError(ScriptResolver{empty.path()}("", "@scripts"), ErrorKind::kModuleNotFound, "scripts/init.luau");
}

TEST(ScriptResolver, SelfIsTheFolderOfAFile) {
    const ScriptsDirectory scripts{"rtype-resolver-self-file"};
    scripts.write("config/main.luau", "");
    scripts.write("config/player.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("config/main.luau", "@self/player")->key, "config/player.luau");
}

TEST(ScriptResolver, SelfIsTheFolderOfAnInit) {
    const ScriptsDirectory scripts{"rtype-resolver-self-init"};
    scripts.write("ui/init.luau", "");
    scripts.write("ui/button.luau", "");
    scripts.write("button.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("ui/init.luau", "@self/button")->key, "ui/button.luau");
}

TEST(ScriptResolver, SelfNeedsARequester) {
    const ScriptsDirectory scripts{"rtype-resolver-self-root"};
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@self/a"), ErrorKind::kInvalidPath, "@self");
}

TEST(ScriptResolver, InAnInitDotMeansTheSiblingsOfItsFolder) {
    const ScriptsDirectory scripts{"rtype-resolver-init-siblings"};
    scripts.write("ui/init.luau", "");
    scripts.write("ui/button.luau", "");
    scripts.write("theme.luau", "");
    scripts.write("lib/util.luau", "");
    const ScriptResolver resolver{scripts.path()};

    EXPECT_EQ(resolver("ui/init.luau", "./theme")->key, "theme.luau");
    EXPECT_EQ(resolver("ui/init.luau", "./lib/util")->key, "lib/util.luau");
    EXPECT_EQ(resolver("ui/init.luau", "./ui/button")->key, "ui/button.luau");
    expectError(resolver("init.luau", "./x"), ErrorKind::kOutsideScripts, "escapes scripts/");
}

TEST(ScriptResolver, UnknownAliasListsTheKnownOnes) {
    const ScriptsDirectory scripts{"rtype-resolver-unknown-alias"};
    const ScriptResolver resolver{scripts.path(), {{"shared", "shared"}, {"Assets", "assets"}}};

    const auto result = resolver("", "@nope/a");
    expectError(result, ErrorKind::kUnknownAlias, "unknown alias '@nope'");
    EXPECT_NE(std::string{result.error().message()}.find("@assets, @scripts, @self, @shared"), std::string::npos)
        << result.error().message();
}

TEST(ScriptResolver, AliasesIgnoreCase) {
    const ScriptsDirectory scripts{"rtype-resolver-alias-case"};
    scripts.write("shared/util.luau", "");
    const ScriptResolver resolver{scripts.path(), {{"Shared", "shared"}}};

    EXPECT_EQ(resolver("", "@shared/util")->key, "shared/util.luau");
    EXPECT_EQ(resolver("", "@SHARED/util")->key, "shared/util.luau");
}

TEST(ScriptResolver, AnExtraAliasPointsToAFolder) {
    const ScriptsDirectory scripts{"rtype-resolver-extra-alias"};
    scripts.write("lib/shared/util.luau", "");
    scripts.write("lib/shared/init.luau", "");
    const ScriptResolver resolver{scripts.path(), {{"shared", "lib/shared/"}}};

    EXPECT_EQ(resolver("a/b.luau", "@shared/util")->key, "lib/shared/util.luau");
    EXPECT_EQ(resolver("a/b.luau", "@shared")->key, "lib/shared/init.luau");
}

TEST(ScriptResolver, RejectsBadAliasesAtConstruction) {
    const ScriptsDirectory scripts{"rtype-resolver-bad-alias"};
    using Aliases = std::map<std::string, std::string>;
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"a/b", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"scripts", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"Self", "x"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"out", "../elsewhere"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"out", "a/../../elsewhere"}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"abs", scripts.path().generic_string()}}}),
                 ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"empty", ""}}}), ScriptLoadException);
    EXPECT_THROW((ScriptResolver{scripts.path(), Aliases{{"a", "x"}, {"A", "y"}}}), ScriptLoadException);
}

TEST(ScriptResolver, RejectsAnAliasThroughALinkPointingOut) {
    const ScriptsDirectory scripts{"rtype-resolver-alias-link"};
    const ScriptsDirectory outside{"rtype-resolver-alias-link-outside"};
    if (!scripts.link("out", outside.path())) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    EXPECT_THROW((ScriptResolver{scripts.path(), {{"out", "out"}}}), ScriptLoadException);
}

TEST(ScriptResolver, RefusesAFileWhoseCaseDiffers) {
    const ScriptsDirectory scripts{"rtype-resolver-case-file"};
    scripts.write("Config.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/config"), ErrorKind::kCaseMismatch, "case mismatch: the file is 'Config.luau'");
    EXPECT_TRUE(resolver("", "@scripts/Config"));
}

TEST(ScriptResolver, RefusesAFolderWhoseCaseDiffers) {
    const ScriptsDirectory scripts{"rtype-resolver-case-folder"};
    scripts.write("Shared/util.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/shared/util"), ErrorKind::kCaseMismatch, "'Shared'");
    EXPECT_TRUE(resolver("", "@scripts/Shared/util"));
}

TEST(ScriptResolver, RefusesALinkedFileOutsideScripts) {
    const ScriptsDirectory scripts{"rtype-resolver-link-out"};
    const ScriptsDirectory outside{"rtype-resolver-link-out-target"};
    outside.write("secret.luau", "return 'secret'");
    if (!scripts.link("secret.luau", outside.path() / "secret.luau") || !scripts.link("dir", outside.path())) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver("", "@scripts/secret"), ErrorKind::kOutsideScripts, "escapes scripts/");
    expectError(resolver("", "@scripts/dir/secret"), ErrorKind::kOutsideScripts, "escapes scripts/");
}

TEST(ScriptResolver, FollowsALinkInsideScripts) {
    const ScriptsDirectory scripts{"rtype-resolver-link-in"};
    scripts.write("real/util.luau", "return 1");
    if (!scripts.link("alias.luau", scripts.path() / "real" / "util.luau")) {
        GTEST_SKIP() << "symbolic links cannot be created here";
    }
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver("", "@scripts/alias");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "alias.luau");
    EXPECT_EQ(module->source, "return 1");
}

TEST(ScriptResolver, ResolvesAnEntry) {
    const ScriptsDirectory scripts{"rtype-resolver-entry"};
    scripts.write("server/main.luau", "return 'server'");
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    const auto module = resolver.resolveEntry("server/main");
    ASSERT_TRUE(module);
    EXPECT_EQ(module->key, "server/main.luau");
    EXPECT_EQ(module->chunkName, "scripts/server/main.luau");
    EXPECT_EQ(module->source, "return 'server'");
    EXPECT_EQ(resolver.resolveEntry("main")->key, "main.luau");
}

TEST(ScriptResolver, RefusesInvalidEntries) {
    const ScriptsDirectory scripts{"rtype-resolver-bad-entry"};
    scripts.write("main.luau", "");
    const ScriptResolver resolver{scripts.path()};

    expectError(resolver.resolveEntry(""), ErrorKind::kInvalidPath, "empty");
    expectError(resolver.resolveEntry("./main"), ErrorKind::kInvalidPath, "no prefix");
    expectError(resolver.resolveEntry("../main"), ErrorKind::kInvalidPath, "no prefix");
    expectError(resolver.resolveEntry("@scripts/main"), ErrorKind::kInvalidPath, "no prefix");
    expectError(resolver.resolveEntry("main.luau"), ErrorKind::kInvalidPath, "extension");
    expectError(resolver.resolveEntry("a/../main"), ErrorKind::kInvalidPath, "invalid entry");
    expectError(resolver.resolveEntry("/main"), ErrorKind::kInvalidPath, "invalid entry");
    expectError(resolver.resolveEntry("main/"), ErrorKind::kInvalidPath, "invalid entry");
    expectError(resolver.resolveEntry("missing"), ErrorKind::kModuleNotFound, "not found");
}
