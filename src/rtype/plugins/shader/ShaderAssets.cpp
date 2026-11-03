/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderAssets
*/

#include "plugins/shader/ShaderAssets.hpp"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/EffectCompiler.hpp"

namespace rtype::plugins::shader {

std::size_t ShaderAssets::getCount() const noexcept { return _shaders.getCount(); }

core::asset::AssetId ShaderAssets::load(const std::filesystem::path& path) {
    const auto stored = _shaders.load(path, [](std::string_view text, const std::filesystem::path& file) {
        return detail::compileEffect(text, core::asset::normalizePath(file));
    });
    if (stored.added != nullptr) {
        _added.push_back(stored.id);
    }
    return stored.id;
}

core::asset::AssetId ShaderAssets::add(const std::string& name, std::string_view source) {
    std::string normalized = core::asset::normalizePath(name);
    const std::optional<core::asset::AssetId> id = _shaders.reserve(normalized);
    if (!id) {
        return core::asset::AssetId::fromPath(normalized);
    }
    try {
        backend::graphics::CompiledShader compiled = detail::compileEffect(source, normalized);
        static_cast<void>(_shaders.store(*id, std::move(normalized), std::move(compiled)));
        _added.push_back(*id);
    } catch (const std::exception& error) {
        _shaders.fail(*id, std::move(normalized), error.what());
    }
    return *id;
}

const backend::graphics::CompiledShader* ShaderAssets::get(core::asset::AssetId id) const { return _shaders.get(id); }

const ShaderAssets::Entry* ShaderAssets::getEntry(core::asset::AssetId id) const { return _shaders.getEntry(id); }

bool ShaderAssets::unload(core::asset::AssetId id) {
    if (!_shaders.unload(id)) {
        return false;
    }
    _unloaded.push_back(id);
    return true;
}

std::vector<core::asset::AssetId> ShaderAssets::takeAdded() { return std::exchange(_added, {}); }

std::vector<core::asset::AssetId> ShaderAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

}  // namespace rtype::plugins::shader
