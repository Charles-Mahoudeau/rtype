/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DefaultPlugins
*/

#include "DefaultPlugins.hpp"

#include <memory>
#include <utility>

#include "core/config/Settings.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"

namespace rtype::engine {
DefaultPlugins::DefaultPlugins(std::unique_ptr<plugins::backend::BackendRegistry> backends,
                               std::unique_ptr<plugins::audio::AudioRegistry> audio, core::config::Settings settings)
    : _backends{std::move(backends)}, _audio{std::move(audio)}, _settings{std::move(settings)} {}

void DefaultPlugins::build(ecs::App& app) {
    app.addPlugins(ecs::MinimalPlugins{})
        .addPlugin(plugins::backend::BackendPlugin{std::move(_backends), _settings})
        .addPlugin(plugins::audio::AudioPlugin{std::move(_audio), _settings});
}
}  // namespace rtype::engine
