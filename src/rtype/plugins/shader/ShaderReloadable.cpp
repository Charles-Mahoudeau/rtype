/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderReloadable
*/

#include "plugins/shader/ShaderReloadable.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"

namespace rtype::plugins::shader {

ShaderReloadable::ShaderReloadable(ShaderAssets* shaders, std::filesystem::path assetDirectory)
    : _shaders{shaders}, _assets{std::move(assetDirectory)} {}

std::string_view ShaderReloadable::name() const noexcept { return "shaders"; }

std::vector<core::reload::WatchedPath> ShaderReloadable::watched() const {
    return {core::reload::WatchedPath{.path = _assets / kDirectory, .extensions = {}}};
}

core::reload::ReloadOutcome ShaderReloadable::reload(const core::reload::ChangeSet& changes) {
    std::vector<std::string> done;
    std::vector<std::string> failed;
    const auto reloadAll = [&](const std::vector<std::string>& files) {
        for (const std::string& file : files) {
            // A path in the assets' directory, as the game loaded the shader.
            const std::filesystem::path path = std::filesystem::path{file}.lexically_relative(_assets);
            const core::asset::AssetId id = core::asset::AssetId::fromPath(path);
            if (_shaders->getEntry(id) == nullptr) {
                continue;  // the game did not load it
            }
            (_shaders->reload(id) ? done : failed).push_back(path.generic_string());
        }
    };
    reloadAll(changes.added);
    reloadAll(changes.modified);

    if (!failed.empty()) {
        std::string names;
        for (const std::string& name : failed) {
            names += std::format("{}{}", names.empty() ? "" : ", ", name);
        }
        return core::reload::ReloadOutcome::failed(
            std::format("{} did not compile, the last version is kept (see the shader log)", names));
    }
    for (std::string& name : done) {
        name = std::format("shader {}", name);
    }
    return core::reload::ReloadOutcome::applied(std::move(done));
}

}  // namespace rtype::plugins::shader
