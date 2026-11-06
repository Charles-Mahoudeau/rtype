/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderAssets
*/

#include "plugins/shader/ShaderAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/shader/detail/ShaderCompiler.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::plugins::shader {

std::size_t ShaderAssets::getCount() const noexcept { return _shaders.getCount(); }

void ShaderAssets::setDirectory(std::filesystem::path directory) { _directory = std::move(directory); }

core::asset::AssetId ShaderAssets::load(const std::filesystem::path& path) {
    std::string name = core::asset::normalizePath(path);
    const std::optional<core::asset::AssetId> id = _shaders.reserve(name);
    if (!id) {
        return core::asset::AssetId::fromPath(name);
    }
    _files.insert(*id);
    try {
        contract::renderer::CompiledShader compiled = compileFile(name);
        static_cast<void>(_shaders.store(*id, std::move(name), std::move(compiled)));
        _added.push_back(*id);
    } catch (const std::exception& error) {
        _shaders.fail(*id, std::move(name), error.what());
    }
    return *id;
}

core::asset::AssetId ShaderAssets::add(const std::string& name, std::string_view source) {
    std::string normalized = core::asset::normalizePath(name);
    const std::optional<core::asset::AssetId> id = _shaders.reserve(normalized);
    if (!id) {
        return core::asset::AssetId::fromPath(normalized);
    }
    try {
        contract::renderer::CompiledShader compiled = detail::compileShader(source, normalized);
        static_cast<void>(_shaders.store(*id, std::move(normalized), std::move(compiled)));
        _added.push_back(*id);
    } catch (const std::exception& error) {
        _shaders.fail(*id, std::move(normalized), error.what());
    }
    return *id;
}

const contract::renderer::CompiledShader* ShaderAssets::get(core::asset::AssetId id) const { return _shaders.get(id); }

const ShaderAssets::Entry* ShaderAssets::getEntry(core::asset::AssetId id) const { return _shaders.getEntry(id); }

bool ShaderAssets::reload(core::asset::AssetId id) {
    const Entry* entry = _files.contains(id) ? _shaders.getEntry(id) : nullptr;
    if (entry == nullptr) {
        return false;
    }
    std::string name = entry->path;
    const bool replaces = _shaders.get(id) != nullptr;
    try {
        contract::renderer::CompiledShader compiled = compileFile(name);
        std::cerr << "[ShaderAssets] Reloaded '" << name << "'\n";
        static_cast<void>(_shaders.store(id, std::move(name), std::move(compiled)));
    } catch (const std::exception& error) {
        if (replaces) {
            std::cerr << "[ShaderAssets] " << error.what() << "\n[ShaderAssets] Keeping the last version of '" << name
                      << "'\n";
        } else {
            _shaders.fail(id, std::move(name), error.what());
        }
        return false;
    }
    std::vector<core::asset::AssetId>& queue = replaces ? _reloaded : _added;
    if (std::ranges::find(queue, id) == queue.end()) {
        queue.push_back(id);
    }
    return true;
}

bool ShaderAssets::unload(core::asset::AssetId id) {
    if (!_shaders.unload(id)) {
        return false;
    }
    _files.erase(id);
    std::erase(_reloaded, id);
    _unloaded.push_back(id);
    return true;
}

std::vector<core::asset::AssetId> ShaderAssets::takeAdded() { return std::exchange(_added, {}); }

std::vector<core::asset::AssetId> ShaderAssets::takeReloaded() { return std::exchange(_reloaded, {}); }

std::vector<core::asset::AssetId> ShaderAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

contract::renderer::CompiledShader ShaderAssets::compileFile(const std::string& name) const {
    std::string text;
    try {
        text = core::asset::readTextFile(_directory / name);
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        throw core::asset::exceptions::AssetLoadException{std::format("Shader '{}': {}", name, error.what())};
    }
    return detail::compileShader(text, name);
}

}  // namespace rtype::plugins::shader
