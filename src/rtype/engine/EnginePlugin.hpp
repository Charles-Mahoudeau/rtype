/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EnginePlugin
*/

#pragma once

#include <filesystem>
#include <string_view>

#include "Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::engine {
/// @brief Starts a game from its project: loads the project's settings, fills the backend and audio registries, and
/// adds the plugins of the App's run mode.
///
/// @details The plugins come from the project's settings when it lists them for the run mode (`plugins.client`,
/// `plugins.server` or `plugins.standalone`, names from the engine's PluginRegistry: "backend", "audio", "physics",
/// "physics.debug", "particle", "text", "tilemap"), and otherwise from DefaultPlugins (client and standalone) or
/// MinimalPlugins (server). The settings script sees `engine.isClient`, `engine.isServer` (from the App's run mode)
/// and `engine.isDebug` (from the build type), and the settings are inserted as the ProjectSettings resource.
///
/// Unlike most plugins, build() loads a file: the settings decide which plugins to add, and the run mode they depend
/// on is only known from the App. Add it first, so the game's plugins and systems can order against the engine's.
/// @code
/// rtype::ecs::App app{rtype::ecs::RunMode::kClient};
/// app.addPlugin(rtype::engine::EnginePlugin{"game/project.luau"});
/// app.addPlugin(GamePlugin{});
/// return app.run();
/// @endcode
class RTYPE_ENGINE_API EnginePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.engine";  ///< The plugin's name.

    /// @param project The project's settings script (`project.luau`), loaded by build().
    explicit EnginePlugin(std::filesystem::path project);
    ~EnginePlugin() override = default;
    EnginePlugin(const EnginePlugin& other) = default;
    EnginePlugin& operator=(const EnginePlugin& other) = default;
    EnginePlugin(EnginePlugin&& other) noexcept = default;
    EnginePlugin& operator=(EnginePlugin&& other) noexcept = default;

    /// @return The project's settings script.
    [[nodiscard]] const std::filesystem::path& getProject() const noexcept;

    /// @return "rtype.engine".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Loads the settings, inserts ProjectSettings, fills the registries and adds the plugins of the run mode.
    /// @throws luau::exceptions::RuntimeError If the script cannot be loaded or run.
    /// @throws exceptions::RuntimeError If the project lists an unknown plugin, or a plugin's settings are wrong.
    void build(ecs::App& app) override;

  private:
    std::filesystem::path _project;  ///< The project's settings script.
};
}  // namespace rtype::engine
