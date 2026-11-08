/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowConfigTests
*/

#include <gtest/gtest.h>

#include <string>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"

using rtype::contract::platform::WindowConfig;

TEST(WindowConfig, LetsGlfwChooseTheWindowSystemByDefault) {
    const rtype::core::config::Settings settings;
    EXPECT_EQ(WindowConfig::fromSettings(settings).system, "auto");
}

TEST(WindowConfig, ReadsTheWindowSystem) {
    for (const std::string system : {"auto", "x11", "wayland"}) {
        rtype::core::config::Settings settings;
        settings.set("system", system);
        EXPECT_EQ(WindowConfig::fromSettings(settings).system, system);
    }
}

TEST(WindowConfig, RefusesAnUnknownWindowSystem) {
    rtype::core::config::Settings settings;
    settings.set("system", std::string{"mir"});
    try {
        static_cast<void>(WindowConfig::fromSettings(settings));
        FAIL() << "accepted";
    } catch (const rtype::core::config::exceptions::SettingsException& error) {
        EXPECT_STREQ(error.what(), R"(Setting 'window.system' must be "auto", "x11" or "wayland", not "mir")");
    }
}
