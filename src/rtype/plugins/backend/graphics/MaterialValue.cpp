/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialValue
*/

#include "plugins/backend/graphics/MaterialValue.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "plugins/backend/exceptions/ShaderExceptions.hpp"
#include "plugins/backend/graphics/Shader.hpp"

namespace rtype::plugins::backend::graphics {

ShaderValueType getValueType(const MaterialValue& value) {
    constexpr std::array<ShaderValueType, std::variant_size_v<MaterialValue>> kTypes{
        ShaderValueType::kFloat, ShaderValueType::kVec2, ShaderValueType::kVec3, ShaderValueType::kVec4,
        ShaderValueType::kInt};
    return kTypes.at(value.index());
}

std::string_view getValueTypeName(ShaderValueType type) noexcept {
    switch (type) {
        case ShaderValueType::kVec2:
            return "vec2";
        case ShaderValueType::kVec3:
            return "vec3";
        case ShaderValueType::kVec4:
            return "vec4";
        case ShaderValueType::kInt:
            return "int";
        case ShaderValueType::kFloat:
            break;
    }
    return "float";
}

void writeShaderValue(std::span<std::byte> bytes, std::uint32_t offset, const MaterialValue& value) {
    std::visit([bytes, offset](
                   const auto& held) { std::memcpy(bytes.subspan(offset, sizeof(held)).data(), &held, sizeof(held)); },
               value);
}

void writeInstanceValue(const CompiledShader& shader, ShaderInstanceValues& values, const std::string& name,
                        const MaterialValue& value) {
    const auto declared = std::ranges::find(shader.instanceValues, name, &ShaderValue::name);
    if (declared == shader.instanceValues.end()) {
        throw exceptions::ShaderException{
            std::format("Instance of '{}': the shader has no instance uniform '{}'", shader.name, name)};
    }
    if (getValueType(value) != declared->type) {
        throw exceptions::ShaderException{std::format("Instance of '{}': '{}' is a {}, not a {}", shader.name, name,
                                                      getValueTypeName(declared->type),
                                                      getValueTypeName(getValueType(value)))};
    }
    writeShaderValue(std::as_writable_bytes(std::span{values}), declared->offset, value);
}

}  // namespace rtype::plugins::backend::graphics
