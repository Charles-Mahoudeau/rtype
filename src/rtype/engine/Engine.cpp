/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#include "Engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "core/config/exceptions/ConfigExceptions.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "rtype/core/config/Settings.hpp"
#include "rtype/core/reload/ReloadReport.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/engine/CommandLine.hpp"
#include "rtype/engine/EngineConfig.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/NetworkInput.hpp"
#include "rtype/engine/plugin/PluginSignature.hpp"
#include "rtype/engine/plugin/RequiredPlugin.hpp"
#include "rtype/engine/reload/ProjectReloadable.hpp"
#include "rtype/engine/reload/ScriptsReloadable.hpp"
#include "rtype/engine/scripting/SettingsLoader.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/plugins/AssetDirectory.hpp"
#include "rtype/plugins/HotReload.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"
#include "rtype/scripting/modules/ScriptResolver.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace {
#ifdef NDEBUG
constexpr bool kIsDebug = false;
#else
constexpr bool kIsDebug = true;
#endif

/// @brief The name of the transform plugin, the one plugin every game has (TransformPlugin::kName, which the Engine
/// does not include; a test checks they are the same).
constexpr std::string_view kTransformPlugin = "rtype.Transform";

/// @return Whether @p mode runs the client side: a window, inputs and sound.
bool runsClient(const rtype::ecs::RunMode mode) { return rtype::ecs::runsIn(rtype::ecs::Side::kClient, mode); }

/// @return Whether @p mode runs the server side.
bool runsServer(const rtype::ecs::RunMode mode) { return rtype::ecs::runsIn(rtype::ecs::Side::kServer, mode); }

/// @return The game directory: `--game`, or @p fallback, made absolute.
/// @throws rtype::engine::exceptions::RuntimeError If it has no project file.
std::filesystem::path findGame(const rtype::engine::CommandLine& commandLine, const std::filesystem::path& fallback) {
    const std::optional<std::string> flag = commandLine.get(rtype::engine::Engine::kGameFlag);
    std::filesystem::path directory = std::filesystem::absolute(flag ? std::filesystem::path{*flag} : fallback);

    directory = directory.lexically_normal();
    if (!std::filesystem::is_regular_file(directory / rtype::engine::Engine::kProjectFile)) {
        throw rtype::engine::exceptions::RuntimeError{
            std::format("No game in '{}': it has no {} (give another directory with --{} <directory>)",
                        directory.string(), rtype::engine::Engine::kProjectFile, rtype::engine::Engine::kGameFlag)};
    }
    return directory;
}

/// @return The settings `project.luau` gives for this mode and command line.
rtype::core::config::Settings loadSettings(const std::filesystem::path& game, const rtype::ecs::RunMode mode,
                                           const rtype::engine::CommandLine& commandLine) {
    rtype::engine::scripting::SettingsLoader loader;

    loader.setFlag("isClient", runsClient(mode));
    loader.setFlag("isServer", runsServer(mode));
    loader.setFlag("isDebug", kIsDebug);
    loader.setArgs(commandLine);
    loader.load(game / rtype::engine::Engine::kProjectFile);
    return loader.build();
}

/// @return The `network.port` setting, checked: a whole number from @p lowest to 65535.
/// @throws rtype::engine::exceptions::RuntimeError If it is not.
std::uint16_t readPort(const rtype::core::config::Settings& network, const std::uint16_t lowest) {
    const double port = network.getNumber("port", rtype::network::NetworkSettings{}.port);

    if (std::trunc(port) != port || port < lowest || port > std::numeric_limits<std::uint16_t>::max()) {
        throw rtype::engine::exceptions::RuntimeError{std::format(
            "Invalid setting 'network.port': {} is not a port (a whole number from {} to 65535)", port, lowest)};
    }
    return static_cast<std::uint16_t>(port);
}

/// @return The `scripts.aliases` setting: alias name, then its target folder in `scripts/`.
/// @throws rtype::engine::exceptions::RuntimeError If a value is not a string, or an alias is invalid (`scripts` and
/// `self` are reserved, names are `a-z 0-9 _ . -`, the target stays inside @p scriptsDirectory). Names the setting.
std::map<std::string, std::string> readScriptAliases(const rtype::core::config::Settings& scripts,
                                                     const std::filesystem::path& scriptsDirectory) {
    const rtype::core::config::Settings aliases = scripts.section("aliases");
    std::map<std::string, std::string> result;

    for (const std::string& name : aliases.keys()) {
        try {
            result.emplace(name, aliases.getString(name, ""));
            // Alone, so the error names this alias; duplicates (names ignore case) fail when the plugin builds.
            const rtype::scripting::modules::ScriptResolver check{scriptsDirectory, {{name, result.at(name)}}};
        } catch (const rtype::core::config::exceptions::SettingsException&) {
            throw rtype::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'scripts.aliases.{}': it must be a string, the folder in scripts/", name)};
        } catch (const rtype::scripting::exceptions::ScriptLoadException& error) {
            throw rtype::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'scripts.aliases.{}': {}", name, error.what())};
        }
    }
    return result;
}

/// @return The server or the client plugin the mode needs, from the `network` settings; null when standalone.
/// @throws rtype::engine::exceptions::RuntimeError If the settings are invalid, or the server's host does not resolve.
std::unique_ptr<rtype::ecs::Plugin> makeNetworkPlugin(const rtype::ecs::RunMode mode,
                                                      const rtype::core::config::Settings& settings) {
    const rtype::core::config::Settings network = settings.section("network");

    network.checkKeys({"host", "port"}, "network");
    if (mode == rtype::ecs::RunMode::kServer) {
        rtype::network::NetworkSettings server;

        server.port = readPort(network, 0);
        return std::make_unique<rtype::replication::server::ServerPlugin>(server,
                                                                          rtype::replication::ReplicationSettings{});
    }
    if (mode == rtype::ecs::RunMode::kClient) {
        const std::string host = network.getString("host", rtype::engine::Engine::kDefaultHost);
        const std::uint16_t port = readPort(network, 1);
        const std::optional<rtype::network::io::Address> address = rtype::network::io::Address::resolve(host, port);

        if (!address) {
            throw rtype::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'network.host': '{}' does not resolve to an address", host)};
        }
        return std::make_unique<rtype::replication::client::ClientPlugin>(*address, rtype::network::NetworkSettings{});
    }
    return nullptr;
}
}  // namespace

namespace rtype::engine {
#ifdef NDEBUG
namespace {
/// @brief Refuses a library of the game that the root key built into this engine does not vouch for.
void verifyGamePlugin(const std::filesystem::path& library) {
    const std::optional<plugin::PublicKey> root = plugin::parsePublicKey(RTYPE_PLUGIN_ROOT_KEY);
    if (!root) {
        throw exceptions::RuntimeError{std::format(
            "Plugin library '{}' cannot be checked: this build has no root key (xmake f --plugin_root_key=<hex>)",
            library.generic_string())};
    }
    plugin::verifyPluginSignature(library, *root);
}
}  // namespace
#endif

Engine::Engine(EngineConfig config)
    : _config{std::move(config)},
      _commandLine{CommandLine::parse(_config.args)},
      _gameDirectory{findGame(_commandLine, _config.game)},
      _reloader{_gameDirectory},
      _hotReload{kIsDebug} {
    build();
}

Engine::~Engine() = default;

ecs::App& Engine::app() noexcept { return *_app; }

const ecs::App& Engine::app() const noexcept { return *_app; }

const CommandLine& Engine::commandLine() const noexcept { return _commandLine; }

const std::filesystem::path& Engine::gameDirectory() const noexcept { return _gameDirectory; }

ecs::RunMode Engine::mode() const noexcept { return _config.mode; }

const core::config::Settings& Engine::settings() const noexcept { return _settings; }

void Engine::requestReload() noexcept { _reloadRequested = true; }

int Engine::run() {
    while (true) {
        if (_app) {
            _app->runFrame();
            if (const std::optional<int> code = _app->takeExit()) {
                return *code;
            }
        } else {
            // The last reload could not build the game: wait for the files to change.
            std::this_thread::sleep_for(core::reload::HotReloader::kInterval / 5);
        }
        pollHotReload();
        if (_reloadRequested.exchange(false)) {
            reload();
        }
    }
}

void Engine::pollHotReload() {
    if (!_hotReload) {
        return;
    }
    // The reloaders log what they did; a rebuild is the only thing left to do here.
    for (const core::reload::ReloadReport& report : _reloader.update(core::reload::HotReloader::Clock::now())) {
        if (report.needsRebuild()) {
            requestReload();
        }
    }
}

void Engine::reload() {
    std::cerr << "[Engine] Reloading the game: the world is built again, its entities, resources and connections "
                 "are lost.\n";
    try {
        build();
    } catch (const std::exception& error) {
        if (!_hotReload) {
            throw;
        }
        _app.reset();
        _scriptHost = nullptr;
        std::cerr << std::format(
            "[Engine] The game cannot be built: {}\n[Engine] Waiting for a change in the game files to try again.\n",
            error.what());
    }
}

void Engine::addHotReloadSources() {
    _reloader.add(std::make_unique<reload::ProjectReloadable>(std::filesystem::path{kProjectFile}, [this] {
        // The project is read again and its network settings checked, without building anything.
        const core::config::Settings settings = loadSettings(_gameDirectory, _config.mode, _commandLine);
        static_cast<void>(makeNetworkPlugin(_config.mode, settings));
    }));
    _reloader.add(std::make_unique<reload::ScriptsReloadable>(_gameDirectory, std::filesystem::path{kScriptsDirectory},
                                                              [this] { return _scriptHost; }));
}

void Engine::registerLibraries() {
    const std::filesystem::path plugins = core::system::getExecutableDirectory() / kPluginsDirectory;
    _loader.load(plugins);
    // Only a client whose game opens a window or plays sound loads what does it: a server never does.
    if (runsClient(_config.mode) &&
        (_settings.has("platform") || _settings.has("renderer") || _settings.has("audio.backend"))) {
        _loader.load(plugins / kBackendsDirectory);
    }
    // A debug build is where plugins are written: only a release build, what players run, checks their signature.
#ifdef NDEBUG
    _loader.load(_gameDirectory / kPluginsDirectory, &verifyGamePlugin);
#else
    _loader.load(_gameDirectory / kPluginsDirectory);
#endif
    ecs::PluginRegistry& registry = _app->getPluginRegistry();
    registry.setSettings(_settings);
    core::library::LibraryRegistrar registrar;
    registrar.provide(ecs::kPluginRegistryService, registry);
    _loader.registerInto(registrar);
}

void Engine::addListedPlugins() {
    const ecs::PluginRegistry& plugins = _app->getPluginRegistry();
    std::vector<std::string> names = _settings.getStringList(kPluginsSetting, {});
    for (const std::string& extra : _config.extraPlugins) {
        if (std::ranges::find(names, extra) == names.end()) {
            names.push_back(extra);
        }
    }
    for (const std::string& name : names) {
        std::unique_ptr<ecs::Plugin> plugin = plugins.create(name);
        if (plugin == nullptr) {
            std::string known;
            for (const std::string& registered : plugins.getNames()) {
                known += std::format("{}'{}'", known.empty() ? "" : ", ", registered);
            }
            throw exceptions::RuntimeError{std::format("Invalid setting '{}': unknown plugin '{}' (known: {})",
                                                       kPluginsSetting, name, known.empty() ? "none" : known)};
        }
        _app->addPlugin(std::move(plugin));
    }
}

void Engine::build() {
    // The old App goes first: its window and its socket must be released before the new one opens them again. What
    // follows files may point into it, so it is forgotten first. The project and the scripts are followed before the
    // project is read, so a project that fails to build is still watched.
    _reloader.clear();
    _scriptHost = nullptr;
    _app.reset();
    addHotReloadSources();
    _settings = loadSettings(_gameDirectory, _config.mode, _commandLine);
    _hotReload = _settings.getBool(kHotReloadSetting, kIsDebug);
    // Checked before anything is built, so a wrong address fails before a window opens; added last, see the class.
    std::unique_ptr<ecs::Plugin> networkPlugin = makeNetworkPlugin(_config.mode, _settings);
    const bool client = runsClient(_config.mode);
    const bool server = runsServer(_config.mode);
    const rtype::core::config::Settings scripts = _settings.section("scripts");
    scripts.checkKeys({"aliases"}, "scripts");
    std::map<std::string, std::string> aliases = readScriptAliases(scripts, _gameDirectory / kScriptsDirectory);
    const std::filesystem::path assets = _gameDirectory / kAssetsDirectory;
    // A server can run with the scripts alone; a client always draws something, so it is checked before a window opens.
    if (client && !std::filesystem::is_directory(assets)) {
        throw exceptions::RuntimeError{
            std::format("No assets in '{}': it has no {} directory", _gameDirectory.string(), kAssetsDirectory)};
    }
    _app = std::make_unique<ecs::App>(_config.mode);

    ecs::App& app = *_app;
    // Inserted first, so every plugin finds it when it is built.
    app.getWorld().getResources().insert(plugins::AssetDirectory{.path = assets});
    if (_hotReload) {
        app.getWorld().getResources().insert(plugins::HotReload{.reloader = &_reloader});
    }

    registerLibraries();
    app.addPlugins(ecs::MinimalPlugins{});
    // Every game has it, and the Engine links it no more than the others: it comes from its library like a listed one.
    app.addPlugin(plugin::createRequiredPlugin(
        app.getPluginRegistry(), kTransformPlugin,
        {core::system::getExecutableDirectory() / kPluginsDirectory, _gameDirectory / kPluginsDirectory}));
    if (_config.addPlugins) {
        _config.addPlugins(app, _settings);
    }
    if (_config.mode != ecs::RunMode::kStandalone) {
        app.addPlugin(replication::ReplicationPlugin{});
    }
    addListedPlugins();
    if (_config.setup) {
        _config.setup(*this);
    }
    app.addPlugin(rtype::scripting::ScriptingPlugin{{
        .directory = _gameDirectory / kScriptsDirectory,
        .aliases = std::move(aliases),
        .options = {.isClient = client, .isServer = server, .isDebug = kIsDebug},
        .onBuilt = [this](rtype::scripting::ScriptHost& host) { _scriptHost = &host; },
    }});
    if (networkPlugin) {
        app.addPlugin(std::move(networkPlugin));
    }
    if (_config.mode == ecs::RunMode::kClient) {
        addNetworkInput(app);
    }
}
}  // namespace rtype::engine
