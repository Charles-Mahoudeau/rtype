/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorPluginInfoTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "editor/panels/PluginInfo.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace {
/// @brief A plugin with a name and the dependencies it is given.
class NamedPlugin : public rtype::ecs::Plugin {
  public:
    NamedPlugin(std::string_view name, std::vector<std::string_view> dependencies)
        : _name{name}, _dependencies{std::move(dependencies)} {}
    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override { return _dependencies; }
    void build(rtype::ecs::App& /*app*/) override {}

  private:
    std::string_view _name;
    std::vector<std::string_view> _dependencies;
};
}  // namespace

TEST(EditorPluginInfo, ListsLoadedPluginsWithRequiresAndRequiredBy) {
    rtype::ecs::App app;
    app.addPlugin(NamedPlugin{"rtype.Base", {}});
    app.addPlugin(NamedPlugin{"game.Boss", {"rtype.Base"}});
    const auto list = rtype::editor::panels::collectPlugins(app, "");
    ASSERT_EQ(list.loaded.size(), 2U);
    EXPECT_EQ(list.loaded[0].name, "rtype.Base");
    EXPECT_TRUE(list.loaded[0].isEngine);
    EXPECT_EQ(list.loaded[0].requiredBy, (std::vector<std::string>{"game.Boss"}));
    EXPECT_EQ(list.loaded[1].requires_, (std::vector<std::string>{"rtype.Base"}));
    EXPECT_FALSE(list.loaded[1].isEngine);
}

TEST(EditorPluginInfo, FiltersByNameIgnoringCaseButKeepsRequiredBy) {
    rtype::ecs::App app;
    app.addPlugin(NamedPlugin{"rtype.Base", {}});
    app.addPlugin(NamedPlugin{"game.Boss", {"rtype.Base"}});
    const auto list = rtype::editor::panels::collectPlugins(app, "BASE");
    ASSERT_EQ(list.loaded.size(), 1U);
    EXPECT_EQ(list.loaded[0].requiredBy, (std::vector<std::string>{"game.Boss"}));
}

TEST(EditorPluginInfo, ListsRegisteredPluginsThatWereNotAdded) {
    rtype::ecs::App app;
    app.getPluginRegistry().add(
        "game.Spare", [] { return std::make_unique<NamedPlugin>("game.Spare", std::vector<std::string_view>{}); });
    app.addPlugin(NamedPlugin{"game.Boss", {}});
    EXPECT_EQ(rtype::editor::panels::collectPlugins(app, "").notLoaded, (std::vector<std::string>{"game.Spare"}));
    app.addPlugin(NamedPlugin{"game.Spare", {}});
    EXPECT_TRUE(rtype::editor::panels::collectPlugins(app, "").notLoaded.empty());
}
