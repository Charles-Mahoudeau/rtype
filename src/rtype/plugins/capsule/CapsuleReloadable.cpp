/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleReloadable
*/

#include "CapsuleReloadable.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ReloadedCapsules.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"

namespace rtype::plugins::capsule {
CapsuleReloadable::CapsuleReloadable(CapsuleFiles* files, Capsules* capsules, ReloadedCapsules* reloaded,
                                     std::filesystem::path assetDirectory)
    : _files{files}, _capsules{capsules}, _reloaded{reloaded}, _assets{std::move(assetDirectory)} {}

std::string_view CapsuleReloadable::name() const noexcept { return "capsules"; }

std::vector<core::reload::WatchedPath> CapsuleReloadable::watched() const {
    return {core::reload::WatchedPath{.path = _assets,
                                      .extensions = {format::kExtensions.begin(), format::kExtensions.end()}}};
}

core::reload::ReloadOutcome CapsuleReloadable::reload(const core::reload::ChangeSet& changes) {
    std::vector<std::string> paths;  // in the assets, as the capsules are named
    for (const auto* files : {&changes.added, &changes.modified}) {
        for (const std::string& file : *files) {
            paths.push_back(std::filesystem::path{file}.lexically_relative(_assets).generic_string());
        }
    }
    CapsuleFiles::Reread reread = _files->reload(*_capsules, paths);
    _reloaded->capsules.insert(_reloaded->capsules.end(), reread.capsules.begin(), reread.capsules.end());
    if (!reread.errors.empty()) {
        std::string message;
        for (const std::string& error : reread.errors) {
            message += std::format("{}{}", message.empty() ? "" : "; ", error);
        }
        return core::reload::ReloadOutcome::failed(std::format("{} (the last version is kept)", message));
    }
    std::vector<std::string> what;
    for (const CapsuleId id : reread.capsules) {
        const Capsule* capsule = _capsules->find(id);
        what.push_back(std::format("capsule {}", capsule != nullptr ? capsule->getPath() : std::string_view{}));
    }
    return core::reload::ReloadOutcome::applied(std::move(what));
}
}  // namespace rtype::plugins::capsule
