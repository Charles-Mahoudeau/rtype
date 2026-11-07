/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineConfig
*/

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "core/config/Settings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

namespace rtype::engine {
class Engine;

/// @brief What an Engine runs, and how it builds its App again on a reload.
///
/// @details The Engine keeps it for its whole life: every build of the App (the first one, then one per reload) calls
/// the functions below again. The Engine names no plugin but the ones every game has: the program lists the others.
/// @code
/// rtype::engine::Engine engine{{
///     .mode = rtype::ecs::RunMode::kClient,
///     .args = {args.begin(), args.end()},
///     .addPlugins = [](rtype::ecs::App& app, const rtype::core::config::Settings& settings) {
///         app.addPlugin(rtype::plugins::input::InputPlugin{});
///         app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{makeBackends(), settings});
///     },
/// }};
/// @endcode
struct EngineConfig {
    /// @brief What the process runs: the client side, the server side, or both (standalone, no network).
    ecs::RunMode mode{ecs::RunMode::kStandalone};

    /// @brief The game directory when the command line has no `--game`; relative to the working directory.
    std::filesystem::path game{"game"};

    /// @brief The command line, without the program name: `--game <dir>`, then any flag `project.luau` reads from
    /// `engine.args`.
    std::vector<std::string> args;

    /// @brief Adds the plugins this program uses, with the project's settings: a server without a window adds none of
    /// the client's, and links none of them. Optional.
    ///
    /// Called on every build of the App, after MinimalPlugins and the transform plugin, before the replication.
    std::function<void(ecs::App&, const core::config::Settings&)> addPlugins;

    /// @brief Plugins added by name after those `project.luau` lists under `plugins`, from their libraries, as if the
    /// project listed them last; one the project already lists is not added twice. For a program that brings a tool
    /// to any game: `aylo-runtime --editor` adds "rtype.Editor".
    std::vector<std::string> extraPlugins;

    /// @brief The game's own native code: its components, resources, systems and plugins. Optional.
    ///
    /// Called on every build of the App, after the engine's plugins and before the scripts and the network plugin,
    /// so the scripts can use what it registers and the network replicates it.
    std::function<void(Engine&)> setup;
};
}  // namespace rtype::engine
