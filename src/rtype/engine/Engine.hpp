/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "Export.hpp"
#include "core/config/Settings.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/PluginRegistry.hpp"

namespace rtype::engine {
/// @brief Entry point of a game: loads the project settings, fills the backend and audio registries, builds the App
/// with the plugins of its run mode, and runs it.
///
/// @details The constructor does it all, so the game then adds its own plugins and systems (which may order against
/// the engine's) before calling run(). The plugins come from the project's settings when it lists them for the run
/// mode (`plugins.client`, `plugins.server` or `plugins.standalone`, names from the engine's PluginRegistry:
/// "backend", "audio", "physics", "physics.debug", "particle", "text", "tilemap"), and otherwise from DefaultPlugins
/// (client and standalone) or MinimalPlugins (server). The settings script sees `engine.isClient`, `engine.isServer`
/// and `engine.isDebug`.
/// @code
/// int main() {
///     rtype::engine::Engine engine{rtype::ecs::RunMode::kClient, "game/project.luau"};
///     engine.getApp().addPlugin(GamePlugin{});
///     return engine.run();
/// }
/// @endcode
class RTYPE_ENGINE_API Engine {
  public:
    /// @param mode What this process runs: a client, a dedicated server, or both (standalone).
    /// @param project The project's settings script (`project.luau`).
    /// @throws luau::exceptions::RuntimeError If the script cannot be loaded or run.
    /// @throws exceptions::RuntimeError If the project lists an unknown plugin, or a plugin's settings are wrong.
    Engine(ecs::RunMode mode, const std::filesystem::path& project);
    ~Engine() = default;
    Engine(const Engine& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(Engine&& other) = delete;

    /// @return The application, to add the game's plugins and systems.
    [[nodiscard]] ecs::App& getApp() noexcept;
    [[nodiscard]] const ecs::App& getApp() const noexcept;

    /// @return The project's settings, as loaded.
    [[nodiscard]] const core::config::Settings& getSettings() const noexcept;

    /// @brief Runs the App until a system sends AppExit (ecs::App::run()).
    /// @return The code of that AppExit.
    int run();

  private:
    /// @brief Registers the plugins a project can list by name.
    void registerPlugins();

    /// @brief Adds the plugins of the run mode: the project's list, or the default ones.
    void addPlugins();

    /// @return The run mode's name in the settings keys ("client", "server", "standalone").
    [[nodiscard]] std::string_view getModeName() const noexcept;

    ecs::RunMode _mode;                                            ///< What this process runs.
    core::config::Settings _settings;                              ///< The project's settings.
    std::unique_ptr<plugins::backend::BackendRegistry> _backends;  ///< Moved into BackendPlugin when it is added.
    std::unique_ptr<plugins::audio::AudioRegistry> _audio;         ///< Moved into AudioPlugin when it is added.
    PluginRegistry _plugins;                                       ///< The plugins a project can list by name.
    ecs::App _app;                                                 ///< The application.
};
}  // namespace rtype::engine
