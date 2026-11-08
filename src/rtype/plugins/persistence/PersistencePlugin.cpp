/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PersistencePlugin
*/

#include "plugins/persistence/PersistencePlugin.hpp"

#include <filesystem>
#include <span>
#include <string_view>
#include <utility>

#include "plugins/persistence/PersistenceCalls.hpp"
#include "plugins/persistence/SaveStore.hpp"
#include "plugins/persistence/UserDataDirectory.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::plugins::persistence {

PersistencePlugin::PersistencePlugin(PersistenceSettings settings) : _settings{std::move(settings)} {}

std::string_view PersistencePlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> PersistencePlugin::getDependencies() const noexcept { return {}; }

void PersistencePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<SaveStore>()) {
        std::filesystem::path directory = _settings.directory;
        if (directory.empty()) {
            directory = getUserDataDirectory() / _settings.folder;
        }
        resources.insert(SaveStore{std::move(directory)});
    }
    addPersistenceCalls(app);
}

}  // namespace rtype::plugins::persistence
