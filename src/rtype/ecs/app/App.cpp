/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** App
*/

#include "App.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
App::App(RunMode mode) : _schedules{_world, mode} {}

App::~App() = default;  // defined here, where Plugin is complete, for the unique_ptrs to destroy it

World& App::getWorld() noexcept { return _world; }

const World& App::getWorld() const noexcept { return _world; }

Schedules& App::getSchedules() noexcept { return _schedules; }

const Schedules& App::getSchedules() const noexcept { return _schedules; }

App& App::addPlugin(std::unique_ptr<Plugin> plugin) {
    assert(plugin != nullptr);
    if (hasPlugin(plugin->getName())) {
        return *this;
    }
    Plugin& added = *_plugins.emplace_back(std::move(plugin));  // recorded first: a nested add of it is skipped
    added.build(*this);
    return *this;
}

bool App::hasPlugin(std::string_view name) const noexcept {
    return std::ranges::any_of(_plugins,
                               [&](const std::unique_ptr<Plugin>& plugin) { return plugin->getName() == name; });
}

void App::runFrame() { _schedules.runFrame(); }

int App::run() {
    while (true) {
        runFrame();
        if (const std::optional<int> code = takeExit()) {
            return *code;
        }
    }
}

std::optional<int> App::takeExit() noexcept {
    for (const AppExit& exit : _world.getEvents().read<AppExit>(_exitCursor)) {
        return exit.code;  // the first one; the cursor is already past every event read
    }
    return std::nullopt;
}
}  // namespace rtype::ecs
