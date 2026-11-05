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

#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/plugins/audio/AudioRegistry.hpp"
#include "rtype/plugins/backend/BackendRegistry.hpp"

namespace rtype::engine {
class Engine;

/// @brief What an Engine runs, and how it builds its App again on a reload.
///
/// @details The Engine keeps it for its whole life: every build of the App (the first one, then one per reload) fills
/// new registries with the functions below, so they register factories, never objects the App would keep.
/// @code
/// rtype::engine::Engine engine{{
///     .mode = rtype::ecs::RunMode::kClient,
///     .args = {args.begin(), args.end()},
///     .registerBackends = [](auto& registry) { rtype::platform::glfw::registerPlatform(registry); },
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

    /// @brief Registers the platforms and renderers the settings can name (client side only).
    std::function<void(plugins::backend::BackendRegistry&)> registerBackends;

    /// @brief Registers the audio backends the settings can name, besides "null" (client side only).
    std::function<void(plugins::audio::AudioRegistry&)> registerAudio;

    /// @brief The game's own native code: its components, resources, systems and plugins. Optional.
    ///
    /// Called on every build of the App, after the engine's plugins and before the scripts and the network plugin,
    /// so the scripts can use what it registers and the network replicates it.
    std::function<void(Engine&)> setup;
};
}  // namespace rtype::engine
