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
#include <iterator>
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

/// @brief What is known of a reload of several files.
struct Summary {
    std::vector<std::string> what;      ///< What was swapped.
    std::vector<std::string> failures;  ///< "file: why" for each file that failed.
    std::string rebuild;                ///< Why a rebuild is needed; empty if none is.
};

/// @brief Adds the outcome of @p file to @p summary.
void add(Summary& summary, const std::string_view file, ReloadOutcome outcome) {
    switch (outcome.status) {
        case ReloadOutcome::Status::kApplied:
            summary.what.insert(summary.what.end(), std::make_move_iterator(outcome.what.begin()),
                                std::make_move_iterator(outcome.what.end()));
            break;
        case ReloadOutcome::Status::kFailed:
            summary.failures.push_back(std::format("{}: {}", file, outcome.message));
            break;
        case ReloadOutcome::Status::kNeedsRebuild:
            if (summary.rebuild.empty()) {
                summary.rebuild = std::format("{}: {}", file, outcome.message);
            }
            break;
    }
}

/// @return The failures, one per line.
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

    Summary summary;
    for (const std::string& file : changes.removed) {
        add(summary, file, host->removeScript(nameOf(file)));
    }
    for (const std::vector<std::string>* files : {&changes.added, &changes.modified}) {
        for (const std::string& file : *files) {
            try {
                add(summary, file, host->reloadScript(nameOf(file), core::asset::readTextFile(_gameDirectory / file)));
            } catch (const std::exception& error) {
                summary.failures.push_back(std::format("{}: {}", file, error.what()));
            }
        }
    }
    if (!summary.rebuild.empty()) {
        return ReloadOutcome::needsRebuild(std::move(summary.rebuild));
    }
    if (!summary.failures.empty()) {
        return ReloadOutcome::failed(joinFailures(summary.failures));
    }
    return ReloadOutcome::applied(std::move(summary.what));
}

}  // namespace rtype::engine::reload
