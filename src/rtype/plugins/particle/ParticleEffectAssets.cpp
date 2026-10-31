/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEffectAssets
*/

#include "plugins/particle/ParticleEffectAssets.hpp"

#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/particle/IParticleEffectLoader.hpp"
#include "plugins/particle/ParticleEffect.hpp"

namespace rtype::plugins::particle {

std::size_t ParticleEffectAssets::getCount() const noexcept { return _effects.getCount(); }

void ParticleEffectAssets::addLoader(std::string extension, std::unique_ptr<IParticleEffectLoader> loader) {
    if (extension.size() < 2 || extension.front() != '.') {
        throw std::invalid_argument{
            std::format(R"(Particle effect loader extension '{}' must start with a dot, as ".luau")", extension)};
    }
    if (!loader) {
        throw std::invalid_argument{std::format("Particle effect loader for '{}' is null", extension)};
    }
    _loaders.insert_or_assign(std::move(extension), std::move(loader));
}

std::vector<std::string> ParticleEffectAssets::getLoaderExtensions() const {
    std::vector<std::string> extensions;
    extensions.reserve(_loaders.size());
    for (const auto& [extension, loader] : _loaders) {
        extensions.push_back(extension);
    }
    return extensions;
}

core::asset::AssetId ParticleEffectAssets::load(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    const auto found = _loaders.find(extension);
    if (found == _loaders.end()) {
        std::string name = core::asset::normalizePath(path);
        const std::optional<core::asset::AssetId> id = _effects.reserve(name);
        if (!id) {
            return core::asset::AssetId::fromPath(name);
        }
        std::string error = std::format("ParticleEffect '{}': no loader for the extension '{}'", name, extension);
        _effects.fail(*id, std::move(name), std::move(error));
        return *id;
    }
    const IParticleEffectLoader& loader = *found->second;
    return _effects
        .load(path,
              [&loader](std::string_view text, const std::filesystem::path& file) {
                  ParticleEffect effect = loader.load(text, file);
                  try {
                      backend::graphics::validate(effect.desc);
                  } catch (const backend::exceptions::ParticleException& error) {
                      throw backend::exceptions::ParticleException{
                          std::format("ParticleEffect '{}': {}", core::asset::normalizePath(file), error.what())};
                  }
                  return effect;
              })
        .id;
}

core::asset::AssetId ParticleEffectAssets::add(const std::string& name, ParticleEffect effect) {
    backend::graphics::validate(effect.desc);
    return _effects.add(name, std::move(effect)).id;
}

const ParticleEffect* ParticleEffectAssets::get(core::asset::AssetId id) const { return _effects.get(id); }

const ParticleEffectAssets::Entry* ParticleEffectAssets::getEntry(core::asset::AssetId id) const {
    return _effects.getEntry(id);
}

bool ParticleEffectAssets::unload(core::asset::AssetId id) { return _effects.unload(id); }

}  // namespace rtype::plugins::particle
