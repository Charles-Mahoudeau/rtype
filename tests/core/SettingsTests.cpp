/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SettingsTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace {

using rtype::core::config::Settings;
using rtype::core::config::exceptions::SettingsException;

}  // namespace

TEST(Settings, HasNothingByDefault) {
    const Settings settings;
    EXPECT_FALSE(settings.has("window.title"));
}

TEST(Settings, SetStoresAndReplacesAValue) {
    Settings settings;
    settings.set("renderer", "vulkan");
    ASSERT_TRUE(settings.has("renderer"));
    EXPECT_EQ(settings.getString("renderer", ""), "vulkan");

    settings.set("renderer", "opengl");
    EXPECT_EQ(settings.getString("renderer", ""), "opengl");
}

TEST(Settings, ReturnsTheFallbackOfAMissingKey) {
    const Settings settings;
    EXPECT_TRUE(settings.getBool("a", true));
    EXPECT_DOUBLE_EQ(settings.getNumber("b", 2.5), 2.5);
    EXPECT_EQ(settings.getString("c", "none"), "none");
    EXPECT_EQ(settings.getNumberList("d", {1.0, 2.0}), (std::vector<double>{1.0, 2.0}));
    EXPECT_EQ(settings.getStringList("e", {"x"}), (std::vector<std::string>{"x"}));
}

TEST(Settings, ReadsEveryType) {
    Settings settings;
    settings.set("flag", true);
    settings.set("count", 3.0);
    settings.set("name", "ship");
    settings.set("size", std::vector<double>{1280.0, 720.0});
    settings.set("tags", std::vector<std::string>{"a", "b"});

    EXPECT_TRUE(settings.getBool("flag", false));
    EXPECT_DOUBLE_EQ(settings.getNumber("count", 0.0), 3.0);
    EXPECT_EQ(settings.getString("name", ""), "ship");
    EXPECT_EQ(settings.getNumberList("size", {}), (std::vector<double>{1280.0, 720.0}));
    EXPECT_EQ(settings.getStringList("tags", {}), (std::vector<std::string>{"a", "b"}));
}

TEST(Settings, RejectsAValueOfAnotherType) {
    Settings settings;
    settings.set("window.title", 42.0);
    settings.set("flag", "yes");
    settings.set("size", "big");

    EXPECT_THROW((void)settings.getString("window.title", ""), SettingsException);
    EXPECT_THROW((void)settings.getBool("flag", false), SettingsException);
    EXPECT_THROW((void)settings.getNumberList("size", {}), SettingsException);
    EXPECT_THROW((void)settings.getStringList("window.title", {}), SettingsException);
}

TEST(Settings, TheErrorNamesTheKey) {
    Settings settings;
    settings.set("window.title", 42.0);
    try {
        (void)settings.getString("window.title", "");
        FAIL() << "expected a SettingsException";
    } catch (const SettingsException& error) {
        EXPECT_NE(std::string{error.what()}.find("window.title"), std::string::npos);
    }
}

TEST(Settings, AnEmptyListReadsAsEitherKindOfList) {
    Settings settings;
    settings.set("numbers", std::vector<std::string>{});
    settings.set("strings", std::vector<double>{});

    EXPECT_TRUE(settings.getNumberList("numbers", {1.0}).empty());
    EXPECT_TRUE(settings.getStringList("strings", {"x"}).empty());
}

TEST(Settings, SectionStripsThePrefix) {
    Settings settings;
    settings.set("window.title", "R-Type");
    settings.set("window.size", std::vector<double>{800.0, 600.0});
    settings.set("renderer", "vulkan");

    const Settings window = settings.section("window");
    EXPECT_EQ(window.getString("title", ""), "R-Type");
    EXPECT_EQ(window.getNumberList("size", {}), (std::vector<double>{800.0, 600.0}));
    EXPECT_FALSE(window.has("renderer"));
}

TEST(Settings, SectionIgnoresKeysThatOnlyShareTheStartOfTheName) {
    Settings settings;
    settings.set("windows.title", "no");
    settings.set("window", "no");

    EXPECT_FALSE(settings.section("window").has("title"));
    EXPECT_FALSE(settings.section("window").has(""));
}

TEST(Settings, CheckKeysAcceptsKnownKeysAndSubsections) {
    Settings settings;
    settings.set("title", "R-Type");
    settings.set("size.width", 800.0);
    EXPECT_NO_THROW(settings.checkKeys({"title", "size"}, "window"));
}

TEST(Settings, CheckKeysAcceptsAnEmptySection) {
    const Settings settings;
    EXPECT_NO_THROW(settings.checkKeys({}, "window"));
}

TEST(Settings, CheckKeysRejectsAnUnknownKeyAndNamesIt) {
    Settings settings;
    settings.set("title", "R-Type");
    settings.set("tittle", "typo");
    try {
        settings.checkKeys({"title"}, "window");
        FAIL() << "expected a SettingsException";
    } catch (const SettingsException& error) {
        const std::string message{error.what()};
        EXPECT_NE(message.find("tittle"), std::string::npos);
        EXPECT_NE(message.find("window"), std::string::npos);
    }
}

TEST(Settings, CheckKeysDoesNotLetAKeyThatSharesAPrefixPass) {
    Settings settings;
    settings.set("sizes", 1.0);
    EXPECT_THROW(settings.checkKeys({"size"}, "window"), SettingsException);
}
