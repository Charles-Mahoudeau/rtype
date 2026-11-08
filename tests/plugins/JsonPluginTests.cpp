/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonPluginTests
*/

#include <gtest/gtest.h>

#include "plugins/json/JsonPlugin.hpp"
#include "rtype/ecs/app/App.hpp"

TEST(JsonPlugin, IsNamedAndRequiresNothing) {
    const rtype::plugins::json::JsonPlugin plugin;
    EXPECT_EQ(plugin.getName(), "rtype.Json");
    EXPECT_TRUE(plugin.getDependencies().empty());
}

TEST(JsonPlugin, AppAcceptsItFirst) {
    rtype::ecs::App app;
    EXPECT_NO_THROW(app.addPlugin(rtype::plugins::json::JsonPlugin{}));
}
