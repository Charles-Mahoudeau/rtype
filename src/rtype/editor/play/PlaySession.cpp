/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlaySession
*/

#include "PlaySession.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "editor/panels/WorldView.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::play {

namespace {
/// @return The entities of @p world that have a component, sorted by their bits.
std::vector<ecs::Entity> entitiesOf(const ecs::World& world) {
    std::vector<ecs::Entity> entities = panels::collectEntities(world, "");
    std::ranges::sort(entities, {}, &ecs::Entity::getBits);
    return entities;
}
}  // namespace

bool PlaySession::isPlaying() const noexcept { return _playing; }

void PlaySession::edit(ecs::App& app) noexcept { app.getSchedules().setPaused(true); }

void PlaySession::play(ecs::App& app) {
    if (_playing) {
        return;
    }
    _before = entitiesOf(app.getWorld());
    _playing = true;
    app.getSchedules().setPaused(false);
}

void PlaySession::togglePause(ecs::App& app) const noexcept {
    if (_playing) {
        app.getSchedules().setPaused(!app.getSchedules().isPaused());
    }
}

void PlaySession::step(ecs::App& app) const noexcept {
    if (_playing) {
        app.getSchedules().step();
    }
}

std::optional<std::string> PlaySession::stop(ecs::App& app, std::optional<plugins::capsule::CapsuleId> scene) {
    if (!_playing) {
        return std::nullopt;
    }
    _playing = false;
    app.getSchedules().setPaused(true);
    ecs::World& world = app.getWorld();
    for (const ecs::Entity entity : entitiesOf(world)) {
        if (!std::ranges::binary_search(_before, entity.getBits(), {}, &ecs::Entity::getBits) &&
            world.isAlive(entity)) {
            static_cast<void>(world.despawn(entity));
        }
    }
    _before.clear();
    if (!scene) {
        return std::nullopt;
    }
    const auto* loaded = world.getResources().get<plugins::capsule::LoadedCapsule>();
    if (loaded != nullptr && loaded->capsule == *scene && world.isAlive(loaded->root)) {
        scene::respawn(world, *scene);
        return std::nullopt;
    }
    return scene::open(world, *scene);
}

}  // namespace rtype::editor::play
