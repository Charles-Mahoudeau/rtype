/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptsReloadable
*/

#include "ScriptsReloadable.hpp"

#include <exception>
#include <filesystem>
#include <format>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/core/asset/AssetFiles.hpp"
#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::engine::reload {

namespace {
using core::reload::ReloadOutcome;

/// @return The failures, joined with "; ".
std::string joinFailures(const std::vector<std::string>& failures) {
    std::string text;
    for (const std::string& failure : failures) {
        text += std::format("{}{}", text.empty() ? "" : "; ", failure);
    }
    return text;
}
}  // namespace

ScriptsReloadable::ScriptsReloadable(std::filesystem::path gameDirectory, std::filesystem::path scriptsDirectory,
                                     HostGetter host)
    : _gameDirectory{std::move(gameDirectory)},
      _scriptsDirectory{std::move(scriptsDirectory)},
      _host{std::move(host)} {}

std::string_view ScriptsReloadable::name() const noexcept { return "scripts"; }

std::vector<core::reload::WatchedPath> ScriptsReloadable::watched() const {
    return {core::reload::WatchedPath{.path = _scriptsDirectory, .extensions = {".luau"}}};
}

core::reload::ReloadOutcome ScriptsReloadable::reload(const core::reload::ChangeSet& changes) {
    scripting::ScriptHost* host = _host();
    if (host == nullptr) {
        return ReloadOutcome::needsRebuild("the game is not running: it is built again");
    }
    // The host knows a script by its chunk name, `scripts/<key>`: the file's path in the game directory.
    const auto nameOf = [](const std::string& file) { return core::asset::normalizePath(file); };

    // One batch: the host runs the changed files and what requires them together, or none of them.
    std::map<std::string, std::string> changed;
    std::vector<std::string> failures;
    for (const std::vector<std::string>* files : {&changes.added, &changes.modified}) {
        for (const std::string& file : *files) {
            try {
                changed.insert_or_assign(nameOf(file), core::asset::readTextFile(_gameDirectory / file));
            } catch (const std::exception& error) {
                failures.push_back(std::format("{}: {}", file, error.what()));
            }
        }
    }
    if (!failures.empty()) {
        return ReloadOutcome::failed(joinFailures(failures));
    }
    std::vector<std::string> removed;
    removed.reserve(changes.removed.size());
    for (const std::string& file : changes.removed) {
        removed.push_back(nameOf(file));
    }
    return host->reloadModules(changed, removed);
}

}  // namespace rtype::engine::reload
