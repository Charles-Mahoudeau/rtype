/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ProjectReloadable
*/

#include "ProjectReloadable.hpp"

#include <exception>
#include <filesystem>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"

namespace rtype::engine::reload {

ProjectReloadable::ProjectReloadable(std::filesystem::path file, Validate validate)
    : _file{std::move(file)}, _validate{std::move(validate)} {}

std::string_view ProjectReloadable::name() const noexcept { return "project"; }

std::vector<core::reload::WatchedPath> ProjectReloadable::watched() const {
    return {core::reload::WatchedPath{.path = _file, .extensions = {}}};
}

core::reload::ReloadOutcome ProjectReloadable::reload(const core::reload::ChangeSet& /*changes*/) {
    try {
        _validate();
    } catch (const std::exception& error) {
        return core::reload::ReloadOutcome::failed(error.what());
    }
    return core::reload::ReloadOutcome::needsRebuild(
        std::format("{} changed: the window, the network and the plugins come from it", _file.generic_string()));
}

}  // namespace rtype::engine::reload
