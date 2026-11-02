/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Shader
*/

#include "plugins/backend/graphics/Shader.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <string_view>

namespace rtype::plugins::backend::graphics {

namespace {

/// @return The bytes a value of @p type takes.
std::uint32_t sizeOf(ShaderValueType type) noexcept {
    switch (type) {
        case ShaderValueType::kVec2:
            return 8;
        case ShaderValueType::kVec3:
            return 12;
        case ShaderValueType::kVec4:
            return 16;
        case ShaderValueType::kFloat:
        case ShaderValueType::kInt:
            break;
    }
    return 4;
}

/// @brief Throws if a value of @p values does not fit in a block of @p blockSize bytes.
void validateValues(const CompiledShader& shader, std::span<const ShaderValue> values, std::size_t blockSize,
                    std::string_view block) {
    for (const ShaderValue& value : values) {
        if (std::size_t{value.offset} + sizeOf(value.type) > blockSize) {
            throw std::invalid_argument{std::format("Shader '{}': the {} value '{}' ends past its {}-byte block",
                                                    shader.name, block, value.name, blockSize)};
        }
    }
}

}  // namespace

const ShaderVariant* CompiledShader::findVariant(ShaderTarget target) const noexcept {
    const auto found = std::ranges::find(variants, target, &ShaderVariant::target);
    return found != variants.end() ? &*found : nullptr;
}

void validate(const CompiledShader& shader) {
    if (shader.variants.empty()) {
        throw std::invalid_argument{std::format("Shader '{}' has no variant: it draws nothing", shader.name)};
    }
    std::array<bool, kShaderTargetCount> seen{};
    for (const ShaderVariant& variant : shader.variants) {
        bool& target = seen.at(static_cast<std::size_t>(variant.target));
        if (target) {
            throw std::invalid_argument{std::format("Shader '{}' has two variants for one target", shader.name)};
        }
        target = true;
        if (variant.vertex.empty() || variant.fragment.empty()) {
            throw std::invalid_argument{std::format("Shader '{}' has a variant with an empty stage", shader.name)};
        }
    }
    if (shader.materialDefaults.size() != shader.materialSize) {
        throw std::invalid_argument{std::format("Shader '{}' has {} bytes of material defaults for a {}-byte block",
                                                shader.name, shader.materialDefaults.size(), shader.materialSize)};
    }
    validateValues(shader, shader.materialValues, shader.materialSize, "material");
    validateValues(shader, shader.instanceValues, kShaderInstanceBytes, "instance");
    std::array<bool, kShaderMaxTextures> slots{};
    for (const ShaderTexture& texture : shader.textures) {
        if (texture.slot >= kShaderMaxTextures) {
            throw std::invalid_argument{std::format("Shader '{}': texture '{}' is in slot {}, past the {} allowed",
                                                    shader.name, texture.name, texture.slot, kShaderMaxTextures)};
        }
        if (slots.at(texture.slot)) {
            throw std::invalid_argument{
                std::format("Shader '{}': texture '{}' shares slot {}", shader.name, texture.name, texture.slot)};
        }
        slots.at(texture.slot) = true;
    }
}

}  // namespace rtype::plugins::backend::graphics
