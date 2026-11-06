/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginEntryTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "core/config/Settings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"

namespace {
/// @brief A plugin that keeps a setting, as one read by a factory.
class ConfiguredPlugin final : public rtype::ecs::Plugin {
  public:
    explicit ConfiguredPlugin(std::string level) : _level{std::move(level)} {}
    ~ConfiguredPlugin() override = default;
    ConfiguredPlugin(const ConfiguredPlugin& other) = default;
    ConfiguredPlugin& operator=(const ConfiguredPlugin& other) = default;
    ConfiguredPlugin(ConfiguredPlugin&& other) noexcept = default;
    ConfiguredPlugin& operator=(ConfiguredPlugin&& other) noexcept = default;

    [[nodiscard]] const std::string& getLevel() const noexcept { return _level; }
    [[nodiscard]] std::string_view getName() const noexcept override { return "test.configured"; }
    void build(rtype::ecs::App& /*app*/) override {}

  private:
    std::string _level;  ///< The setting read.
};
}  // namespace

RTYPE_PLUGIN_LIBRARY(registrar) {
    registrar.getPlugins().add("test.configured", [](const rtype::core::config::Settings& settings) {
        return std::make_unique<ConfiguredPlugin>(settings.getString("test.level", "none"));
    });
}

TEST(PluginEntry, ALibraryPassesItsOwnCheck) {
    EXPECT_NO_THROW(rtype::ecs::checkPluginLibrary(*rtypePluginLibrary(), "test"));
}

TEST(PluginEntry, ALibraryRegistersItsPluginsWithTheSettings) {
    rtype::ecs::PluginRegistry registry;
    rtype::core::config::Settings settings;
    settings.set("test.level", "hard");
    registry.setSettings(settings);
    rtype::ecs::PluginRegistrar registrar{registry};
    rtypePluginLibrary()->registerPlugins(registrar);

    const std::unique_ptr<rtype::ecs::Plugin> plugin = registry.create("test.configured");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(dynamic_cast<ConfiguredPlugin&>(*plugin).getLevel(), "hard");
}

TEST(PluginEntry, RefusesAnotherVersionAndNamesTheFile) {
    rtype::ecs::PluginLibrary library = *rtypePluginLibrary();
    library.abiVersion = rtype::ecs::kPluginAbiVersion + 1;
    try {
        rtype::ecs::checkPluginLibrary(library, "boss.so");
        FAIL() << "the library was accepted";
    } catch (const rtype::ecs::exceptions::PluginException& error) {
        EXPECT_TRUE(std::string_view{error.what()}.contains("boss.so"));
    }
}

TEST(PluginEntry, RefusesAnotherToolchain) {
    rtype::ecs::PluginLibrary library = *rtypePluginLibrary();
    library.toolchain = "another compiler";
    EXPECT_THROW(rtype::ecs::checkPluginLibrary(library, "boss.so"), rtype::ecs::exceptions::PluginException);
    library.toolchain = nullptr;
    EXPECT_THROW(rtype::ecs::checkPluginLibrary(library, "boss.so"), rtype::ecs::exceptions::PluginException);
}
