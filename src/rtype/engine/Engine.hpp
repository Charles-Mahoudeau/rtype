/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <string_view>

#include "CommandLine.hpp"
#include "EngineConfig.hpp"
#include "Export.hpp"
#include "rtype/core/config/Settings.hpp"
#include "rtype/core/reload/HotReloader.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/plugin/PluginLoader.hpp"

namespace rtype::scripting {
class ScriptHost;
}  // namespace rtype::scripting

namespace rtype::engine {
/// @brief Runs a game: reads its project settings, builds the App with the engine's plugins, then runs it.
///
/// @details A game is a directory (`game` by default, `--game <dir>` on the command line) holding `project.luau` (the
/// settings, see scripting::SettingsLoader) and `scripts/` (the Luau files of scripting::ScriptingPlugin). Each build
/// of the App adds, in this order:
/// 1. the MinimalPlugins and the transform plugin (from its library), which every game has;
/// 2. the plugins of the program, EngineConfig::addPlugins, with the settings: a client adds its window and what it
///    draws there, so the Engine names no backend and a server links none;
/// 3. on a network side (client and server modes), the replication plugin;
/// 4. the plugins the settings name under `plugins`, built from the plugin libraries of the `plugins` folders, next to
///    the executable and in the game directory; those they require come with them;
/// 5. the game's native code, EngineConfig::setup;
/// 6. the scripting plugin, which runs the game's scripts;
/// 7. the network plugin, server or client, last because it reads every replicated component when it is built;
/// 8. on a client, the system that fills the input sent to the server from the input actions (see addNetworkInput()).
///
/// The network reads the `network` section of the settings: `port` on both sides (4242 by default; 0 lets a server
/// pick a free one) and `host` on a client ("127.0.0.1" by default).
///
/// requestReload() reads the project again and builds a new App from it, between two frames. The world is not kept:
/// its entities, resources and connections are lost, and the window is closed and opened again.
///
/// Hot reload (the `hotReload` setting, on by default in a debug build) follows the game's files while it runs, with a
/// core::reload::HotReloader the engine looks at between two frames: a change to `project.luau` builds the App again; a
/// change to a script swaps its systems and components in the live world (scripting::ScriptHost::reloadScript()), and
/// builds the App again only when it cannot; a plugin follows its own files by adding a core::reload::Reloadable to the
/// reloader (see plugins::HotReload). Each reload is logged: what, why, which files, how long. If a rebuild fails the
/// engine says why and waits for the next change to try again, instead of stopping.
/// @code
/// rtype::engine::Engine engine{{.mode = rtype::ecs::RunMode::kServer, .args = {"--port", "4243"}}};
/// return engine.run();
/// @endcode
class RTYPE_ENGINE_API Engine {
  public:
    static constexpr std::string_view kGameFlag = "game";             ///< The flag that names the game directory.
    static constexpr std::string_view kProjectFile = "project.luau";  ///< The settings, in the game directory.
    static constexpr std::string_view kScriptsDirectory = "scripts";  ///< The scripts, in the game directory.
    static constexpr std::string_view kAssetsDirectory = "assets";    ///< The assets, in the game directory.
    static constexpr std::string_view kPluginsDirectory =
        "plugins";  ///< The libraries, by the executable and the game.
    static constexpr std::string_view kBackendsDirectory =
        "backends";                                                 ///< The backends, in the plugins by the executable.
    static constexpr std::string_view kPluginsSetting = "plugins";  ///< The setting naming the plugins to add.
    static constexpr std::string_view kDefaultHost = "127.0.0.1";   ///< The server a client joins by default.
    static constexpr std::string_view kHotReloadSetting =
        "hotReload";  ///< The setting that turns hot reload on or off.

    /// @brief Reads the command line and the project settings, then builds the App.
    /// @param config What to run; kept to build the App again on a reload.
    /// @throws exceptions::RuntimeError If the command line is invalid, the game has no `project.luau`, the network
    /// settings are invalid, or `plugins` names an unknown plugin.
    /// @throws core::config::exceptions::SettingsException If the settings of a plugin are invalid.
    /// @throws luau::exceptions::RuntimeError If `project.luau` fails.
    /// @throws std::exception What a plugin, a script or EngineConfig::setup throws while the App is built.
    explicit Engine(EngineConfig config);
    ~Engine();
    Engine(const Engine& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(Engine&& other) = delete;

    /// @return The App the engine runs; a new one after each reload. Only valid while the game is built: after a
    /// hot reload whose build failed there is none until the files are fixed (run() is then waiting).
    [[nodiscard]] ecs::App& app() noexcept;
    [[nodiscard]] const ecs::App& app() const noexcept;

    /// @return The flags of the command line.
    [[nodiscard]] const CommandLine& commandLine() const noexcept;

    /// @return The game directory, absolute.
    [[nodiscard]] const std::filesystem::path& gameDirectory() const noexcept;

    /// @return What the process runs.
    [[nodiscard]] ecs::RunMode mode() const noexcept;

    /// @return The settings `project.luau` gave at the last build.
    [[nodiscard]] const core::config::Settings& settings() const noexcept;

    /// @brief Asks run() to read the project again and build a new App after the current frame.
    ///
    /// Safe to call from a system, or from another thread (a file watcher). The world is lost: see the class.
    void requestReload() noexcept;

    /// @brief Runs frames until a system sends AppExit, building a new App between two frames when a reload was
    /// requested.
    /// @return The code of the first AppExit read.
    /// @throws std::exception What a frame throws, or what a reload throws (the engine then has no App).
    int run();

  private:
    /// @brief Reads the project settings and builds a new App from them, replacing the current one.
    void build();

    /// @brief Builds the App again between two frames. With hot reload on, a build that fails is reported and leaves
    /// the engine without an App, waiting for the files to change; otherwise the error goes to the caller.
    void reload();

    /// @brief Looks at the files hot reload follows and asks for a reload if one of them needs it.
    void pollHotReload();

    /// @brief Makes the reloader follow `project.luau` and the scripts.
    void addHotReloadSources();

    /// @brief Loads the plugin libraries, once, and registers them in the App, with the settings.
    void registerLibraries();

    /// @brief Adds the plugins the settings name.
    /// @throws exceptions::RuntimeError If a name is not registered.
    void addListedPlugins();

    plugin::PluginLoader _loader;          ///< The plugin libraries; outlives every App built from them.
    EngineConfig _config;                  ///< What to run, kept for the reloads.
    CommandLine _commandLine;              ///< The flags of EngineConfig::args.
    std::filesystem::path _gameDirectory;  ///< The game directory, absolute.
    core::reload::HotReloader _reloader;   ///< What hot reload follows; outlives the Apps, forgets them on a build.
    scripting::ScriptHost* _scriptHost{nullptr};  ///< The scripts of the current App, null when there is none.
    bool _hotReload{false};                       ///< Whether hot reload is on, from the settings of the last build.
    core::config::Settings _settings;             ///< The settings of the last build.
    std::unique_ptr<ecs::App> _app;               ///< The App; replaced by a reload.
    std::atomic<bool> _reloadRequested{false};    ///< Set by requestReload(), cleared by the reload.
};
}  // namespace rtype::engine
