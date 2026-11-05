/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderHandles
*/

#include "plugins/shader/ShaderHandles.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

namespace rtype::plugins::shader {

const ShaderHandles::Material* ShaderHandles::getMaterial(core::asset::AssetId id) const {
    const auto found = _materials.find(id);
    return found != _materials.end() ? &found->second : nullptr;
}

bool ShaderHandles::hasShader(core::asset::AssetId id) const { return _shaders.contains(id); }

void ShaderHandles::update(ShaderAssets& shaders, MaterialAssets& materials, const backend::TextureAssets& textures,
                           backend::graphics::IRenderer& renderer) {
    const std::vector<core::asset::AssetId> unloadedMaterials = materials.takeUnloaded();
    const std::vector<core::asset::AssetId> unloadedShaders = shaders.takeUnloaded();
    const std::vector<core::asset::AssetId> addedShaders = shaders.takeAdded();
    const std::vector<core::asset::AssetId> changedMaterials = materials.takeChanged();
    if (!renderer.supportsShaders()) {
        return;
    }
    for (const core::asset::AssetId id : unloadedMaterials) {
        destroyMaterial(id, renderer);
        std::erase(_waiting, id);
    }
    for (const core::asset::AssetId id : unloadedShaders) {
        destroyShader(id, renderer);
    }
    for (const core::asset::AssetId id : addedShaders) {
        const backend::graphics::CompiledShader* compiled = shaders.get(id);
        if (compiled == nullptr || _shaders.contains(id)) {
            continue;
        }
        try {
            _shaders.emplace(id, renderer.createShader(*compiled));
        } catch (const std::exception& error) {
            std::cerr << "[ShaderHandles] Shader '" << compiled->name << "': " << error.what() << '\n';
        }
    }
    for (const core::asset::AssetId id : changedMaterials) {
        if (std::ranges::find(_waiting, id) == _waiting.end()) {
            _waiting.push_back(id);
        }
    }
    std::erase_if(_waiting,
                  [&](core::asset::AssetId id) { return createMaterial(id, shaders, materials, textures, renderer); });
}

void ShaderHandles::destroyMaterial(core::asset::AssetId id, backend::graphics::IRenderer& renderer) {
    const auto found = _materials.find(id);
    if (found != _materials.end()) {
        renderer.destroy(found->second.material);
        _materials.erase(found);
    }
}

void ShaderHandles::destroyShader(core::asset::AssetId id, backend::graphics::IRenderer& renderer) {
    const auto found = _shaders.find(id);
    if (found == _shaders.end()) {
        return;
    }
    for (auto material = _materials.begin(); material != _materials.end();) {
        if (material->second.shader != id) {
            ++material;
            continue;
        }
        renderer.destroy(material->second.material);
        if (std::ranges::find(_waiting, material->first) == _waiting.end()) {
            _waiting.push_back(material->first);
        }
        material = _materials.erase(material);
    }
    renderer.destroy(found->second);
    _shaders.erase(found);
}

bool ShaderHandles::createMaterial(core::asset::AssetId id, const ShaderAssets& shaders,
                                   const MaterialAssets& materials, const backend::TextureAssets& textures,
                                   backend::graphics::IRenderer& renderer) {
    const shader::Material* material = materials.get(id);
    if (material == nullptr) {
        return true;
    }
    const auto shaderId = _shaders.find(material->shader);
    const backend::graphics::CompiledShader* compiled = shaders.get(material->shader);
    if (shaderId == _shaders.end() || compiled == nullptr) {
        return false;
    }
    try {
        backend::graphics::MaterialDesc desc{.shader = shaderId->second,
                                             .values = packMaterialValues(*compiled, *material)};
        const auto slots = resolveMaterialTextures(*compiled, *material);
        for (std::size_t slot = 0; slot < slots.size(); ++slot) {
            if (slots.at(slot) == core::asset::AssetId{}) {
                continue;
            }
            const backend::TextureAssets::Entry* entry = textures.get(slots.at(slot));
            if (entry != nullptr && entry->state == backend::AssetState::kQueued) {
                return false;
            }
            const std::optional<backend::graphics::TextureId> texture = textures.getTexture(slots.at(slot));
            if (!texture) {
                throw exceptions::ShaderException{"one of its textures failed to load or was never asked for"};
            }
            desc.textures.at(slot) = *texture;
        }
        const auto created = _materials.find(id);
        if (created != _materials.end() && created->second.shader == material->shader) {
            renderer.updateMaterial(created->second.material, desc);
            return true;
        }
        destroyMaterial(id, renderer);
        _materials.emplace(id, Material{.material = renderer.createMaterial(desc),
                                        .shader = material->shader,
                                        .instanceDefaults = compiled->instanceDefaults});
    } catch (const std::exception& error) {
        std::cerr << "[ShaderHandles] Material '" << materials.getEntry(id)->path << "': " << error.what() << '\n';
    }
    return true;
}

}  // namespace rtype::plugins::shader
