/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialValue
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "plugins/backend/graphics/Shader.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief A value of a material or of an `instance uniform`, of one of the types a shader declares: float, vec2,
/// vec3, vec4 or int.
using MaterialValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, std::int32_t>;

/// @return The ShaderValueType @p value holds.
[[nodiscard]] ShaderValueType getValueType(const MaterialValue& value);

/// @return The GLSL name of @p type ("float", "vec2"...), for messages.
[[nodiscard]] std::string_view getValueTypeName(ShaderValueType type) noexcept;

/// @brief Copies @p value into @p bytes at @p offset, as a shader lays it out.
void writeShaderValue(std::span<std::byte> bytes, std::uint32_t offset, const MaterialValue& value);

/// @brief Writes @p value at the place of @p shader's `instance uniform` @p name in @p values.
/// @throws exceptions::ShaderException If @p shader has no `instance uniform` of this name (a `uniform` is set in a
/// material) or @p value is not of its type.
void writeInstanceValue(const CompiledShader& shader, ShaderInstanceValues& values, const std::string& name,
                        const MaterialValue& value);

}  // namespace rtype::plugins::backend::graphics
