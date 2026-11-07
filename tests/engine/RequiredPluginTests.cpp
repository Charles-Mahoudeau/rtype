/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RequiredPluginTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/plugin/RequiredPlugin.hpp"
#include "rtype/plugins/transform/TransformPlugin.hpp"

TEST(RequiredPlugin, FailsClearlyWhenNoLibraryProvidesIt) {
    const rtype::ecs::PluginRegistry registry;

    try {
        [[maybe_unused]] const auto plugin = rtype::engine::plugin::createRequiredPlugin(
            registry, rtype::plugins::transform::TransformPlugin::kName, {std::filesystem::path{"some/plugins"}});
        FAIL() << "expected an error";
    } catch (const rtype::engine::exceptions::RuntimeError& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find(std::string{rtype::plugins::transform::TransformPlugin::kName}), std::string::npos);
        EXPECT_NE(message.find("some/plugins"), std::string::npos);
    }
}

TEST(RequiredPlugin, BuildsThePluginAnyLibraryRegistered) {
    rtype::ecs::PluginRegistry registry;
    registry.add(std::string{rtype::plugins::transform::TransformPlugin::kName},
                 [] { return std::make_unique<rtype::plugins::transform::TransformPlugin>(); });

    const auto plugin =
        rtype::engine::plugin::createRequiredPlugin(registry, rtype::plugins::transform::TransformPlugin::kName, {});

    EXPECT_NE(plugin, nullptr);
}
