/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DefaultPlugins
*/

#pragma once

#include <memory>

#include "Export.hpp"
#include "core/config/Settings.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "rtype/ecs/app/PluginGroup.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::engine {
/// @brief The plugins of a windowed game when its project lists none: MinimalPlugins (time), BackendPlugin (window and
/// renderer; it brings input, transform, sprite, tilemap, text and particle) and AudioPlugin.
///
/// @details The engine builds it with the backend and audio registries it filled and the project's settings, and adds
/// it to a client (or standalone) App; a dedicated server gets MinimalPlugins only. The registries are moved into the
/// plugins, so the group is added once.
/// @code
/// app.addPlugins(rtype::engine::DefaultPlugins{std::move(backends), std::move(audio), settings});
/// @endcode
class RTYPE_ENGINE_API DefaultPlugins : public ecs::PluginGroup {
  public:
    /// @param backends The platforms and renderers by name, for BackendPlugin.
    /// @param audio The audio backends by name, for AudioPlugin.
    /// @param settings The project's settings, read by both plugins.
    DefaultPlugins(std::unique_ptr<plugins::backend::BackendRegistry> backends,
                   std::unique_ptr<plugins::audio::AudioRegistry> audio, core::config::Settings settings);
    ~DefaultPlugins() override = default;
    DefaultPlugins(const DefaultPlugins& other) = delete;
    DefaultPlugins& operator=(const DefaultPlugins& other) = delete;
    DefaultPlugins(DefaultPlugins&& other) noexcept = default;
    DefaultPlugins& operator=(DefaultPlugins&& other) noexcept = default;

    /// @brief Adds MinimalPlugins, BackendPlugin and AudioPlugin, moving the registries into them.
    void build(ecs::App& app) override;

  private:
    std::unique_ptr<plugins::backend::BackendRegistry> _backends;  ///< Moved into BackendPlugin by build().
    std::unique_ptr<plugins::audio::AudioRegistry> _audio;         ///< Moved into AudioPlugin by build().
    core::config::Settings _settings;                              ///< Given to both plugins.
};
}  // namespace rtype::engine
