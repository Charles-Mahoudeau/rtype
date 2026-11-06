/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Material
*/

#include "plugins/shader/Material.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/MaterialValue.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

namespace rtype::plugins::shader {

Material& Material::set(const std::string& name, MaterialValue value) {
    const auto found = std::ranges::find(values, name, &Value::name);
    if (found != values.end()) {
        found->value = value;
    } else {
        values.push_back(Value{.name = name, .value = value});
    }
    return *this;
}

Material& Material::setTexture(const std::string& name, core::asset::AssetId texture) {
    const auto found = std::ranges::find(textures, name, &Texture::name);
    if (found != textures.end()) {
        found->texture = texture;
    } else {
        textures.push_back(Texture{.name = name, .texture = texture});
    }
    return *this;
}

std::vector<std::byte> packMaterialValues(const backend::graphics::CompiledShader& shader, const Material& material) {
    std::vector<std::byte> block = shader.materialDefaults;
    for (const Material::Value& set : material.values) {
        const auto declared = std::ranges::find(shader.materialValues, set.name, &backend::graphics::ShaderValue::name);
        if (declared == shader.materialValues.end()) {
            throw exceptions::ShaderException{
                std::format("Material of '{}': the shader has no uniform '{}'", shader.name, set.name)};
        }
        if (backend::graphics::getValueType(set.value) != declared->type) {
            throw exceptions::ShaderException{
                std::format("Material of '{}': '{}' is a {}, not a {}", shader.name, set.name,
                            backend::graphics::getValueTypeName(declared->type),
                            backend::graphics::getValueTypeName(backend::graphics::getValueType(set.value)))};
        }
        backend::graphics::writeShaderValue(block, declared->offset, set.value);
    }
    return block;
}

std::array<core::asset::AssetId, backend::graphics::kShaderMaxTextures> resolveMaterialTextures(
    const backend::graphics::CompiledShader& shader, const Material& material) {
    std::array<core::asset::AssetId, backend::graphics::kShaderMaxTextures> slots{};
    for (const Material::Texture& set : material.textures) {
        const auto declared = std::ranges::find(shader.textures, set.name, &backend::graphics::ShaderTexture::name);
        if (declared == shader.textures.end()) {
            throw exceptions::ShaderException{
                std::format("Material of '{}': the shader has no texture '{}'", shader.name, set.name)};
        }
        slots.at(declared->slot) = set.texture;
    }
    return slots;
}

}  // namespace rtype::plugins::shader
