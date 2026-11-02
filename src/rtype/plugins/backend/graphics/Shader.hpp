/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Shader
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

#include "plugins/backend/graphics/Texture.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief A shader created by IRenderer::createShader(): a slot in the renderer's table and its generation, as
/// TextureId. Local to one renderer.
struct ShaderId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no shader".

    [[nodiscard]] bool operator==(const ShaderId& other) const = default;
};

/// @brief A material created by IRenderer::createMaterial(): a shader and its values. 0 is the default material, the
/// built-in effect of each drawing.
struct MaterialId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "the default material".

    [[nodiscard]] bool operator==(const MaterialId& other) const = default;
};

/// @brief What an effect is drawn on: each has its own inputs, so an effect is compiled once per target it serves.
enum class ShaderTarget : std::uint8_t {
    kSprite,    ///< Sprites and the tiles of a tilemap: a textured quad.
    kGlyph,     ///< The glyphs of a Text: an MSDF atlas, read as distances.
    kParticle,  ///< Particles, of a CPU or a GPU simulation.
};

/// @brief Number of ShaderTarget values.
inline constexpr std::size_t kShaderTargetCount = 3;

/// @brief The type of a value an effect declares (`uniform float flash;`), as the shader reads it (std140 in a
/// material, std430 in an instance).
enum class ShaderValueType : std::uint8_t {
    kFloat,
    kVec2,
    kVec3,
    kVec4,
    kInt,
};

/// @brief Bytes of the values a drawing gives its own copy of (`instance uniform`): 4 vec4, carried with each sprite,
/// glyph or particle draw so that one enemy can flash without its own material.
inline constexpr std::size_t kShaderInstanceBytes = 64;

/// @brief Textures an effect may read besides the drawing's own (`uniform sampler2D noise;`), given by its material.
inline constexpr std::size_t kShaderMaxTextures = 3;

/// @brief The `instance uniform` values of one drawing, as the shader reads them: kShaderInstanceBytes bytes.
using ShaderInstanceValues = std::array<glm::vec4, kShaderInstanceBytes / sizeof(glm::vec4)>;

/// @brief One value an effect declares, found by reflection.
struct ShaderValue {
    std::string name;                               ///< As written in the effect.
    ShaderValueType type{ShaderValueType::kFloat};  ///< Its type.
    std::uint32_t offset{0};                        ///< Bytes from the start of its block (material or instance).

    [[nodiscard]] bool operator==(const ShaderValue& other) const = default;
};

/// @brief One extra texture an effect declares, found by reflection.
struct ShaderTexture {
    std::string name;       ///< As written in the effect.
    std::uint32_t slot{0};  ///< Index in MaterialDesc::textures: 0 to kShaderMaxTextures - 1.

    [[nodiscard]] bool operator==(const ShaderTexture& other) const = default;
};

/// @brief An effect's compiled code for one target: SPIR-V, the format every backend starts from (Vulkan uses it as
/// is, others translate it, e.g. with SPIRV-Cross).
struct ShaderVariant {
    ShaderTarget target{ShaderTarget::kSprite};  ///< What it draws.
    std::vector<std::uint32_t> vertex;           ///< The vertex stage, SPIR-V words.
    std::vector<std::uint32_t> fragment;         ///< The fragment stage, SPIR-V words.
};

/// @brief An effect compiled by the shader plugin, given to IRenderer::createShader(): its code for each target it
/// serves, and what reflection found in it. Plain data: it says nothing of any graphics API.
struct CompiledShader {
    std::string name;                         ///< For messages: its file, or the name it was added under.
    std::vector<ShaderVariant> variants;      ///< One per target, each target at most once.
    std::vector<ShaderValue> materialValues;  ///< `uniform` values, in the material's block.
    std::uint32_t materialSize{0};            ///< Bytes of the material's block.
    std::vector<std::byte> materialDefaults;  ///< The block filled with the declared defaults: materialSize bytes.
    std::vector<ShaderValue> instanceValues;  ///< `instance uniform` values, in a drawing's ShaderInstanceValues.
    ShaderInstanceValues instanceDefaults{};  ///< The declared defaults of the instance values.
    std::vector<ShaderTexture> textures;      ///< `uniform sampler2D` textures, in slot order.

    /// @return The variant for @p target, or nullptr if the effect does not serve it.
    [[nodiscard]] const ShaderVariant* findVariant(ShaderTarget target) const noexcept;
};

/// @brief A shader and the values of its `uniform`s and textures, given to IRenderer::createMaterial(). Drawings that
/// share a material share its values; each still has its own `instance uniform` values.
struct MaterialDesc {
    ShaderId shader;                ///< From IRenderer::createShader().
    std::vector<std::byte> values;  ///< The material's block: CompiledShader::materialSize bytes.
    std::array<TextureId, kShaderMaxTextures> textures{};  ///< By slot; an unused slot stays a default TextureId.
};

/// @brief Checks @p shader can be created: at least one variant, no target twice, no empty stage, every value inside
/// its block, the defaults of the block's size, and texture slots below kShaderMaxTextures, each used once.
/// @throws std::invalid_argument Naming the shader and what is wrong.
void validate(const CompiledShader& shader);

}  // namespace rtype::plugins::backend::graphics
