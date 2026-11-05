/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** App
*/

#include "App.hpp"

#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
App::App(RunMode mode) : _schedules{_world, mode} {}

App::~App() = default;  // defined here, where Plugin is complete, for the unique_ptrs to destroy it

World& App::getWorld() noexcept { return _world; }

const World& App::getWorld() const noexcept { return _world; }

Schedules& App::getSchedules() noexcept { return _schedules; }

const Schedules& App::getSchedules() const noexcept { return _schedules; }

PluginRegistry& App::getPluginRegistry() noexcept { return _pluginRegistry; }

const PluginRegistry& App::getPluginRegistry() const noexcept { return _pluginRegistry; }

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the plugins' dependency tree, cycles are refused.
App& App::addPlugin(std::unique_ptr<Plugin> plugin) {
    assert(plugin != nullptr);
    if (hasPlugin(plugin->getName())) {
        return *this;
    }
    addDependencies(*plugin);
    if (hasPlugin(plugin->getName())) {  // a dependency added it from its build()
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

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the plugins' dependency tree, cycles are refused.
void App::addDependencies(const Plugin& plugin) {
    _resolving.emplace_back(plugin.getName());
    try {
        for (const std::string_view dependency : plugin.getDependencies()) {
            if (hasPlugin(dependency)) {
                continue;
            }
            if (const auto cycle = std::ranges::find(_resolving, dependency); cycle != _resolving.end()) {
                std::string path;
                for (auto name = cycle; name != _resolving.end(); ++name) {
                    path += std::format("'{}' -> ", *name);
                }
                throw exceptions::PluginException{std::format("Plugin dependency cycle: {}'{}'", path, dependency)};
            }
            std::unique_ptr<Plugin> required = _pluginRegistry.create(dependency);
            if (required == nullptr) {
                std::string registered;
                for (const std::string& name : _pluginRegistry.getNames()) {
                    registered += std::format("{}'{}'", registered.empty() ? "" : ", ", name);
                }
                throw exceptions::PluginException{
                    std::format("Plugin '{}' requires '{}', which was not added and is not registered; registered: {}",
                                plugin.getName(), dependency, registered.empty() ? "none" : registered)};
            }
            if (required->getName() != dependency) {
                throw exceptions::PluginException{
                    std::format("Plugin '{}' requires '{}', but the factory registered under that name builds '{}'",
                                plugin.getName(), dependency, required->getName())};
            }
            std::cerr << std::format("[App] Plugin '{}' added because '{}' requires it\n", dependency,
                                     plugin.getName());
            addPlugin(std::move(required));
        }
    } catch (...) {
        _resolving.pop_back();  // leaves the App able to add other plugins after the error
        throw;
    }
    _resolving.pop_back();
}

void App::runFrame() {
    if (!hasPlugin(TimePlugin::kName)) {  // every App has time: the default one unless a configured one was added
        addPlugin(TimePlugin{});
    }
    _schedules.runFrame();
}

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
