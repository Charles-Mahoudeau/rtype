/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialAssets
*/

#include "plugins/shader/MaterialAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/Material.hpp"

namespace rtype::plugins::shader {

std::size_t MaterialAssets::getCount() const noexcept { return _materials.getCount(); }

core::asset::AssetId MaterialAssets::add(const std::string& name, Material material) {
    const auto stored = _materials.add(name, std::move(material));
    if (stored.added != nullptr) {
        markChanged(stored.id);
    }
    return stored.id;
}

const Material* MaterialAssets::get(core::asset::AssetId id) const { return _materials.get(id); }

const MaterialAssets::Entry* MaterialAssets::getEntry(core::asset::AssetId id) const { return _materials.getEntry(id); }

bool MaterialAssets::setValue(core::asset::AssetId id, const std::string& name, MaterialValue value) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.set(name, value);
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

bool MaterialAssets::setTexture(core::asset::AssetId id, const std::string& name, core::asset::AssetId texture) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.setTexture(name, texture);
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

bool MaterialAssets::unload(core::asset::AssetId id) {
    if (!_materials.unload(id)) {
        return false;
    }
    std::erase(_changed, id);
    _unloaded.push_back(id);
    return true;
}

std::vector<core::asset::AssetId> MaterialAssets::takeChanged() { return std::exchange(_changed, {}); }

std::vector<core::asset::AssetId> MaterialAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

void MaterialAssets::markChanged(core::asset::AssetId id) {
    if (std::ranges::find(_changed, id) == _changed.end()) {
        _changed.push_back(id);
    }
}

}  // namespace rtype::plugins::shader
