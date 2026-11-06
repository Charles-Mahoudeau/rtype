/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#include "Engine.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "rtype/core/config/Settings.hpp"
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
#include "rtype/engine/scripting/SettingsLoader.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/plugins/AssetDirectory.hpp"
#include "rtype/plugins/transform/TransformPlugin.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace {
#ifdef NDEBUG
constexpr bool kIsDebug = false;
#else
constexpr bool kIsDebug = true;
#endif

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
Engine::Engine(EngineConfig config)
    : _config{std::move(config)},
      _commandLine{CommandLine::parse(_config.args)},
      _gameDirectory{findGame(_commandLine, _config.game)} {
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
        _app->runFrame();
        if (const std::optional<int> code = _app->takeExit()) {
            return *code;
        }
        if (_reloadRequested.exchange(false)) {
            std::cerr << "[Engine] Reloading the game: the world is built again, its entities, resources and "
                         "connections are lost.\n";
            build();
        }
    }
}

void Engine::registerLibraries() {
    const std::filesystem::path plugins = core::system::getExecutableDirectory() / kPluginsDirectory;
    _loader.load(plugins);
    // Only a client whose game opens a window or plays sound loads what does it: a server never does.
    if (runsClient(_config.mode) &&
        (_settings.has("platform") || _settings.has("renderer") || _settings.has("audio.backend"))) {
        _loader.load(plugins / kBackendsDirectory);
    }
    _loader.load(_gameDirectory / kPluginsDirectory);
    ecs::PluginRegistry& registry = _app->getPluginRegistry();
    registry.setSettings(_settings);
    core::library::LibraryRegistrar registrar;
    registrar.provide(ecs::kPluginRegistryService, registry);
    _loader.registerInto(registrar);
}

void Engine::addListedPlugins() {
    const ecs::PluginRegistry& plugins = _app->getPluginRegistry();
    for (const std::string& name : _settings.getStringList(kPluginsSetting, {})) {
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
    // The old App goes first: its window and its socket must be released before the new one opens them again.
    _app.reset();
    _settings = loadSettings(_gameDirectory, _config.mode, _commandLine);
    // Checked before anything is built, so a wrong address fails before a window opens; added last, see the class.
    std::unique_ptr<ecs::Plugin> networkPlugin = makeNetworkPlugin(_config.mode, _settings);
    const bool client = runsClient(_config.mode);
    const bool server = runsServer(_config.mode);
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

    registerLibraries();
    app.addPlugins(ecs::MinimalPlugins{});
    app.addPlugin(plugins::transform::TransformPlugin{});
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
    app.addPlugin(::rtype::scripting::ScriptingPlugin{{
        .directory = _gameDirectory / kScriptsDirectory,
        .options = {.isClient = client, .isServer = server, .isDebug = kIsDebug},
    }});
    if (networkPlugin) {
        app.addPlugin(std::move(networkPlugin));
    }
    if (_config.mode == ecs::RunMode::kClient) {
        addNetworkInput(app);
    }
}
}  // namespace rtype::engine
