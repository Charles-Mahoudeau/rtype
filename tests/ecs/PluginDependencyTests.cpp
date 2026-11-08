/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginDependencyTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"

namespace rtype::test::plugin_dependency {
/// @brief The names of the plugins built, in build order.
using BuildLog = std::vector<std::string>;

/// @brief A plugin requiring the plugins it is given, and logging its build.
class RequiringPlugin : public rtype::ecs::Plugin {
  public:
    RequiringPlugin(std::string_view name, std::vector<std::string_view> dependencies, BuildLog* log)
        : _name{name}, _dependencies{std::move(dependencies)}, _log{log} {}
    ~RequiringPlugin() override = default;
    RequiringPlugin(const RequiringPlugin& other) = default;
    RequiringPlugin& operator=(const RequiringPlugin& other) = default;
    RequiringPlugin(RequiringPlugin&& other) noexcept = default;
    RequiringPlugin& operator=(RequiringPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override { return _dependencies; }
    void build(rtype::ecs::App& /*app*/) override { _log->emplace_back(_name); }

  private:
    std::string _name;                            ///< The plugin's name.
    std::vector<std::string_view> _dependencies;  ///< The plugins it requires.
    BuildLog* _log;                               ///< Where its build is logged.
};

/// @brief A plugin without declared dependencies.
class PlainPlugin : public rtype::ecs::Plugin {
  public:
    PlainPlugin() = default;
    ~PlainPlugin() override = default;
    PlainPlugin(const PlainPlugin& other) = default;
    PlainPlugin& operator=(const PlainPlugin& other) = default;
    PlainPlugin(PlainPlugin&& other) noexcept = default;
    PlainPlugin& operator=(PlainPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "test.plain"; }
    void build(rtype::ecs::App& /*app*/) override {}
};

/// @brief Registers a RequiringPlugin factory in the App's registry.
void registerPlugin(rtype::ecs::App& app, std::string_view name, std::vector<std::string_view> dependencies,
                    BuildLog* log) {
    app.getPluginRegistry().add(std::string{name}, [name, dependencies = std::move(dependencies), log] {
        return std::make_unique<RequiringPlugin>(name, dependencies, log);
    });
}

/// @return The message of the PluginException thrown by adding the plugin, or an empty string if none is.
std::string addError(rtype::ecs::App& app, RequiringPlugin plugin) {
    try {
        app.addPlugin(std::move(plugin));
    } catch (const rtype::ecs::exceptions::PluginException& error) {
        return error.what();
    }
    return "";
}
}  // namespace rtype::test::plugin_dependency

namespace pdep = rtype::test::plugin_dependency;

TEST(PluginDependency, PluginsDeclareNoDependenciesByDefault) {
    EXPECT_TRUE(pdep::PlainPlugin{}.getDependencies().empty());
}

TEST(PluginDependency, AnAddedDependencyIsNotBuiltAgain) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    app.addPlugin(pdep::RequiringPlugin{"test.b", {}, &log});
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");  // nothing was missing
    EXPECT_EQ(log, (pdep::BuildLog{"test.b", "test.a"}));
}

TEST(PluginDependency, BuildsAMissingDependencyFromTheRegistryFirst) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.b", {}, &log);
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    const std::string warning = testing::internal::GetCapturedStderr();
    EXPECT_EQ(log, (pdep::BuildLog{"test.b", "test.a"}));
    EXPECT_TRUE(app.hasPlugin("test.b"));
    EXPECT_NE(warning.find("'test.b' added because 'test.a' requires it"), std::string::npos) << warning;
}

TEST(PluginDependency, AddingTheDependencyAfterwardsDoesNothing) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.b", {}, &log);
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    app.addPlugin(pdep::RequiringPlugin{"test.b", {}, &log});
    static_cast<void>(testing::internal::GetCapturedStderr());
    EXPECT_EQ(log, (pdep::BuildLog{"test.b", "test.a"}));
}

TEST(PluginDependency, ResolvesTransitiveDependencies) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.b", {"test.c"}, &log);
    pdep::registerPlugin(app, "test.c", {}, &log);
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    static_cast<void>(testing::internal::GetCapturedStderr());
    EXPECT_EQ(log, (pdep::BuildLog{"test.c", "test.b", "test.a"}));
}

TEST(PluginDependency, BuildsASharedDependencyOnce) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.b", {"test.d"}, &log);
    pdep::registerPlugin(app, "test.c", {"test.d"}, &log);
    pdep::registerPlugin(app, "test.d", {}, &log);
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.b", "test.c"}, &log});
    static_cast<void>(testing::internal::GetCapturedStderr());
    EXPECT_EQ(log, (pdep::BuildLog{"test.d", "test.b", "test.c", "test.a"}));
}

TEST(PluginDependency, RefusesADependencyNeitherAddedNorRegistered) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.other", {}, &log);
    const std::string message = pdep::addError(app, pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    EXPECT_EQ(message,
              "Plugin 'test.a' requires 'test.b', which was not added and is not registered; registered: 'test.other'");
    EXPECT_FALSE(app.hasPlugin("test.a"));
    EXPECT_TRUE(log.empty());
}

TEST(PluginDependency, SaysWhenNoPluginIsRegistered) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    const std::string message = pdep::addError(app, pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    EXPECT_NE(message.find("registered: none"), std::string::npos) << message;
}

TEST(PluginDependency, RefusesACycleAndShowsItsPath) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.b", {"test.c"}, &log);
    pdep::registerPlugin(app, "test.c", {"test.a"}, &log);
    pdep::registerPlugin(app, "test.a", {"test.b"}, &log);
    testing::internal::CaptureStderr();
    const std::string message = pdep::addError(app, pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    static_cast<void>(testing::internal::GetCapturedStderr());
    EXPECT_EQ(message, "Plugin dependency cycle: 'test.a' -> 'test.b' -> 'test.c' -> 'test.a'");
    EXPECT_FALSE(app.hasPlugin("test.a"));
    EXPECT_TRUE(log.empty());
}

TEST(PluginDependency, RefusesAPluginRequiringItself) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    const std::string message = pdep::addError(app, pdep::RequiringPlugin{"test.a", {"test.a"}, &log});
    EXPECT_EQ(message, "Plugin dependency cycle: 'test.a' -> 'test.a'");
}

TEST(PluginDependency, RefusesAFactoryBuildingAnotherPlugin) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    app.getPluginRegistry().add("test.b", [&log] {
        return std::make_unique<pdep::RequiringPlugin>("test.c", std::vector<std::string_view>{}, &log);
    });
    const std::string message = pdep::addError(app, pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    EXPECT_EQ(message, "Plugin 'test.a' requires 'test.b', but the factory registered under that name builds 'test.c'");
    EXPECT_FALSE(app.hasPlugin("test.c"));
}

TEST(PluginDependency, StaysUsableAfterAnError) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    static_cast<void>(pdep::addError(app, pdep::RequiringPlugin{"test.a", {"test.missing"}, &log}));
    pdep::registerPlugin(app, "test.missing", {}, &log);
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.missing"}, &log});
    static_cast<void>(testing::internal::GetCapturedStderr());
    EXPECT_EQ(log, (pdep::BuildLog{"test.missing", "test.a"}));
}

TEST(PluginDependency, GetPluginsListsThePluginsInTheOrderTheyWereRecorded) {
    rtype::ecs::App app;
    pdep::BuildLog log;
    pdep::registerPlugin(app, "test.b", {}, &log);
    testing::internal::CaptureStderr();
    app.addPlugin(pdep::RequiringPlugin{"test.a", {"test.b"}, &log});
    app.addPlugin(pdep::RequiringPlugin{"test.a", {}, &log});  // already added: ignored
    static_cast<void>(testing::internal::GetCapturedStderr());
    std::vector<std::string> names;
    for (const rtype::ecs::Plugin* plugin : app.getPlugins()) {
        names.emplace_back(plugin->getName());
    }
    EXPECT_EQ(names, (std::vector<std::string>{"test.b", "test.a"}));
}
