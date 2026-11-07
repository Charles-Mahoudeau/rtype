/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptingPlugin
*/

#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "rtype/core/asset/AssetFiles.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/scripting/ScriptHost.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"
#include "rtype/scripting/modules/ScriptResolver.hpp"

namespace rtype::scripting {
namespace {
constexpr std::string_view kScriptExtension = ".luau";

/// @return The keys of the `.luau` files under @p directory (paths relative to it, with their extension and `/`),
/// sorted; none, with a warning, if it is not a directory.
/// @throws exceptions::ScriptLoadException If @p directory cannot be read.
std::vector<std::string> findScripts(const std::filesystem::path& directory) {
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        std::cerr << std::format("[ScriptingPlugin] Scripts directory '{}' not found, no script loaded\n",
                                 directory.string());
        return {};
    }
    std::vector<std::string> keys;
    for (std::filesystem::recursive_directory_iterator it{directory, error}, end; !error && it != end;
         it.increment(error)) {
        if (it->is_regular_file() && it->path().extension() == kScriptExtension) {
            keys.push_back(core::asset::normalizePath(it->path().lexically_relative(directory)));
        }
    }
    if (error) {
        throw exceptions::ScriptLoadException{
            std::format("Cannot read scripts directory '{}': {}", directory.string(), error.message())};
    }
    // Sorted on the portable form, so the order does not depend on the platform's separator.
    std::ranges::sort(keys);
    return keys;
}
}  // namespace

ScriptingPlugin::ScriptingPlugin(Config config) : _config{std::move(config)} {}

ScriptingPlugin::~ScriptingPlugin() = default;

ScriptingPlugin::ScriptingPlugin(ScriptingPlugin&& other) noexcept = default;

ScriptingPlugin& ScriptingPlugin::operator=(ScriptingPlugin&& other) noexcept = default;

std::string_view ScriptingPlugin::getName() const noexcept { return "rtype.scripting"; }

void ScriptingPlugin::build(ecs::App& app) {
    const std::vector<std::string> keys = findScripts(_config.directory);
    auto host = ScriptHost::create(app, _config.options, modules::ScriptResolver{_config.directory}.forLoader());
    if (!host) {
        throw exceptions::ScriptLoadException{
            std::format("Cannot create the script runtime: {}", host.error().message())};
    }
    _host = std::move(*host);

    // Every file is a root require, so a file another one required is already loaded and does not run again.
    for (const std::string& key : keys) {
        const std::string path = std::format("./{}", key.substr(0, key.size() - kScriptExtension.size()));
        if (auto result = _host->require(path); !result) {
            throw exceptions::ScriptLoadException{
                std::format("Script 'scripts/{}' failed: {}", key, result.error().message())};
        }
    }
    if (_config.onBuilt) {
        _config.onBuilt(*_host);
    }
}
}  // namespace rtype::scripting
