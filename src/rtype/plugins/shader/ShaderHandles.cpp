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
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::plugins::shader {

const ShaderHandles::Material* ShaderHandles::getMaterial(core::asset::AssetId id) const {
    const auto found = _materials.find(id);
    return found != _materials.end() ? &found->second : nullptr;
}

bool ShaderHandles::hasShader(core::asset::AssetId id) const { return _shaders.contains(id); }

void ShaderHandles::update(ShaderAssets& shaders, MaterialAssets& materials, const graphics::TextureAssets& textures,
                           contract::renderer::IRenderer& renderer) {
    const std::vector<core::asset::AssetId> unloadedMaterials = materials.takeUnloaded();
    const std::vector<core::asset::AssetId> unloadedShaders = shaders.takeUnloaded();
    const std::vector<core::asset::AssetId> addedShaders = shaders.takeAdded();
    const std::vector<core::asset::AssetId> reloadedShaders = shaders.takeReloaded();
    const std::vector<core::asset::AssetId> changedMaterials = materials.takeChanged();
    if (!renderer.supportsShaders()) {
        return;
    }
    for (const core::asset::AssetId id : unloadedMaterials) {
        destroyMaterial(id, renderer);
        std::erase(_waiting, id);
        _rejected.erase(id);
    }
    for (const core::asset::AssetId id : unloadedShaders) {
        destroyShader(id, renderer);
    }
    for (const core::asset::AssetId id : addedShaders) {
        const contract::renderer::CompiledShader* compiled = shaders.get(id);
        if (compiled == nullptr || _shaders.contains(id)) {
            continue;
        }
        try {
            _shaders.emplace(id, renderer.createShader(*compiled));
        } catch (const std::exception& error) {
            std::cerr << "[ShaderHandles] Shader '" << compiled->name << "': " << error.what() << '\n';
        }
    }
    for (const core::asset::AssetId id : reloadedShaders) {
        replaceShader(id, shaders, renderer);
    }
    for (const core::asset::AssetId id : changedMaterials) {
        if (std::ranges::find(_waiting, id) == _waiting.end()) {
            _waiting.push_back(id);
        }
    }
    std::erase_if(_waiting,
                  [&](core::asset::AssetId id) { return createMaterial(id, shaders, materials, textures, renderer); });
}

void ShaderHandles::destroyMaterial(core::asset::AssetId id, contract::renderer::IRenderer& renderer) {
    const auto found = _materials.find(id);
    if (found != _materials.end()) {
        renderer.destroy(found->second.material);
        _materials.erase(found);
    }
}

void ShaderHandles::requeueMaterials(core::asset::AssetId id, contract::renderer::IRenderer& renderer) {
    const auto wait = [this](core::asset::AssetId material) {
        if (std::ranges::find(_waiting, material) == _waiting.end()) {
            _waiting.push_back(material);
        }
    };
    for (auto material = _materials.begin(); material != _materials.end();) {
        if (material->second.shader != id) {
            ++material;
            continue;
        }
        renderer.destroy(material->second.material);
        wait(material->first);
        material = _materials.erase(material);
    }
    for (const auto& [material, shader] : _rejected) {
        if (shader == id) {
            wait(material);
        }
    }
}

void ShaderHandles::destroyShader(core::asset::AssetId id, contract::renderer::IRenderer& renderer) {
    const auto found = _shaders.find(id);
    if (found == _shaders.end()) {
        return;
    }
    requeueMaterials(id, renderer);
    renderer.destroy(found->second);
    _shaders.erase(found);
}

void ShaderHandles::replaceShader(core::asset::AssetId id, const ShaderAssets& shaders,
                                  contract::renderer::IRenderer& renderer) {
    const contract::renderer::CompiledShader* compiled = shaders.get(id);
    if (compiled == nullptr) {
        return;
    }
    contract::renderer::ShaderId created;
    try {
        created = renderer.createShader(*compiled);
    } catch (const std::exception& error) {
        std::cerr << "[ShaderHandles] Shader '" << compiled->name << "': " << error.what() << '\n';
        return;
    }
    requeueMaterials(id, renderer);
    if (const auto found = _shaders.find(id); found != _shaders.end()) {
        renderer.destroy(found->second);
    }
    _shaders.insert_or_assign(id, created);
}

bool ShaderHandles::createMaterial(core::asset::AssetId id, const ShaderAssets& shaders,
                                   const MaterialAssets& materials, const graphics::TextureAssets& textures,
                                   contract::renderer::IRenderer& renderer) {
    const shader::Material* material = materials.get(id);
    if (material == nullptr) {
        return true;
    }
    _rejected.erase(id);
    const auto shaderId = _shaders.find(material->shader);
    const contract::renderer::CompiledShader* compiled = shaders.get(material->shader);
    if (shaderId == _shaders.end() || compiled == nullptr) {
        return false;
    }
    try {
        contract::renderer::MaterialDesc desc{.shader = shaderId->second,
                                              .values = packMaterialValues(*compiled, *material)};
        const auto slots = resolveMaterialTextures(*compiled, *material);
        for (std::size_t slot = 0; slot < slots.size(); ++slot) {
            if (slots.at(slot) == core::asset::AssetId{}) {
                continue;
            }
            const graphics::TextureAssets::Entry* entry = textures.get(slots.at(slot));
            if (entry != nullptr && entry->state == graphics::AssetState::kQueued) {
                return false;
            }
            const std::optional<contract::renderer::TextureId> texture = textures.getTexture(slots.at(slot));
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
        _rejected.insert_or_assign(id, material->shader);
    }
    return true;
}

}  // namespace rtype::plugins::shader
