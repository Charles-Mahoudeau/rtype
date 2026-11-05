/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginRegistryTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/PluginRegistry.hpp"

namespace {
using rtype::engine::PluginRegistry;

/// @brief A plugin that only has a name.
class NamedPlugin : public rtype::ecs::Plugin {
  public:
    explicit NamedPlugin(std::string name) : _name{std::move(name)} {}
    ~NamedPlugin() override = default;
    NamedPlugin(const NamedPlugin& other) = default;
    NamedPlugin& operator=(const NamedPlugin& other) = default;
    NamedPlugin(NamedPlugin&& other) noexcept = default;
    NamedPlugin& operator=(NamedPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }
    void build(rtype::ecs::App& /*app*/) override {}

  private:
    std::string _name;  ///< The plugin's name.
};

/// @return A factory building a NamedPlugin called @p name.
PluginRegistry::Factory makeFactory(std::string name) {
    return [name = std::move(name)] { return std::make_unique<NamedPlugin>(name); };
}
}  // namespace

TEST(PluginRegistry, IsEmptyByDefault) {
    const PluginRegistry registry;
    EXPECT_TRUE(registry.getNames().empty());
    EXPECT_FALSE(registry.has("backend"));
}

TEST(PluginRegistry, CreatesThePluginRegisteredUnderAName) {
    PluginRegistry registry;
    registry.add("particle", makeFactory("test.particle"));
    ASSERT_TRUE(registry.has("particle"));
    const std::unique_ptr<rtype::ecs::Plugin> plugin = registry.create("particle");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->getName(), "test.particle");
}

TEST(PluginRegistry, BuildsANewPluginEachTime) {
    PluginRegistry registry;
    registry.add("particle", makeFactory("test.particle"));
    const std::unique_ptr<rtype::ecs::Plugin> first = registry.create("particle");
    const std::unique_ptr<rtype::ecs::Plugin> second = registry.create("particle");
    EXPECT_NE(first.get(), second.get());
}

TEST(PluginRegistry, ListsItsNamesSorted) {
    PluginRegistry registry;
    registry.add("text", makeFactory("test.text"));
    registry.add("audio", makeFactory("test.audio"));
    registry.add("physics", makeFactory("test.physics"));
    EXPECT_EQ(registry.getNames(), (std::vector<std::string>{"audio", "physics", "text"}));
}

TEST(PluginRegistry, ReplacesAFactoryRegisteredTwice) {
    PluginRegistry registry;
    registry.add("audio", makeFactory("test.first"));
    registry.add("audio", makeFactory("test.second"));
    EXPECT_EQ(registry.create("audio")->getName(), "test.second");
    EXPECT_EQ(registry.getNames().size(), 1U);
}

TEST(PluginRegistry, RejectsAnUnknownNameNamingItAndTheKnownOnes) {
    PluginRegistry registry;
    registry.add("audio", makeFactory("test.audio"));
    registry.add("text", makeFactory("test.text"));
    try {
        static_cast<void>(registry.create("audoi"));
        FAIL() << "no exception";
    } catch (const rtype::engine::exceptions::RuntimeError& error) {
        const std::string_view message{error.what()};
        EXPECT_NE(message.find("'audoi'"), std::string_view::npos) << message;
        EXPECT_NE(message.find("'audio', 'text'"), std::string_view::npos) << message;
    }
}
