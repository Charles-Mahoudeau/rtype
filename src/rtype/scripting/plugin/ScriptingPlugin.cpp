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
#include "rtype/core/asset/exceptions/AssetExceptions.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"

namespace rtype::scripting {
namespace {
constexpr std::string_view kScriptExtension = ".luau";

/// @return The `.luau` files under @p directory, sorted by path; none, with a warning, if it is not a directory.
/// @throws exceptions::ScriptLoadException If @p directory cannot be read.
std::vector<std::filesystem::path> findScripts(const std::filesystem::path& directory) {
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        std::cerr << std::format("[ScriptingPlugin] Scripts directory '{}' not found, no script loaded\n",
                                 directory.string());
        return {};
    }
    std::vector<std::filesystem::path> scripts;
    for (std::filesystem::recursive_directory_iterator it{directory, error}, end; !error && it != end;
         it.increment(error)) {
        if (it->is_regular_file() && it->path().extension() == kScriptExtension) {
            scripts.push_back(it->path());
        }
    }
    if (error) {
        throw exceptions::ScriptLoadException{
            std::format("Cannot read scripts directory '{}': {}", directory.string(), error.message())};
    }
    // Sorted on the portable form, so the order does not depend on the platform's separator.
    std::ranges::sort(scripts, {}, [](const std::filesystem::path& path) { return core::asset::normalizePath(path); });
    return scripts;
}
}  // namespace

ScriptingPlugin::ScriptingPlugin(Config config) : _config{std::move(config)} {}

ScriptingPlugin::~ScriptingPlugin() = default;

ScriptingPlugin::ScriptingPlugin(ScriptingPlugin&& other) noexcept = default;

ScriptingPlugin& ScriptingPlugin::operator=(ScriptingPlugin&& other) noexcept = default;

std::string_view ScriptingPlugin::getName() const noexcept { return "rtype.scripting"; }

void ScriptingPlugin::build(ecs::App& app) {
    auto host = ScriptHost::create(app, _config.options);
    if (!host) {
        throw exceptions::ScriptLoadException{
            std::format("Cannot create the script runtime: {}", host.error().message())};
    }
    _host = std::move(*host);

    for (const auto& path : findScripts(_config.directory)) {
        const std::string name = core::asset::normalizePath(path);
        std::string source;
        try {
            source = core::asset::readTextFile(path);
        } catch (const core::asset::exceptions::AssetLoadException& e) {
            throw exceptions::ScriptLoadException{std::format("Cannot read script '{}': {}", name, e.what())};
        }
        if (auto result = _host->run(name, source); !result) {
            throw exceptions::ScriptLoadException{
                std::format("Script '{}' failed: {}", name, result.error().message())};
        }
    }
}
}  // namespace rtype::scripting
