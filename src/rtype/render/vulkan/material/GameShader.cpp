/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameShader
*/

#include "GameShader.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/Spirv.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::render::vulkan::material {

namespace {

/// @brief What a target is called in the name of its pipeline, in ShaderTarget order.
constexpr std::array<std::string_view, contract::renderer::kShaderTargetCount> kTargetNames{"sprite", "glyph",
                                                                                            "particle", "shape"};

/// @return A module named @p name for @p spirv, checked first.
vk::raii::ShaderModule createModule(const core::Device& device, std::span<const std::uint32_t> spirv,
                                    std::string_view name) {
    pipeline::validateSpirv(spirv, name);
    vk::raii::ShaderModule created{device.getDevice(), vk::ShaderModuleCreateInfo{{}, spirv}};
    device.setName(*created, name);
    return created;
}

}  // namespace

GameShader::GameShader(const core::Device& device, const contract::renderer::CompiledShader& shader)
    : _name{shader.name}, _materialSize{shader.materialSize}, _instanceValues{!shader.instanceValues.empty()} {
    contract::renderer::validate(shader);
    for (const contract::renderer::ShaderVariant& variant : shader.variants) {
        Variant& created = _variants.at(static_cast<std::size_t>(variant.target));
        created.vertex = createModule(device, variant.vertex, std::format("{} (vertex)", shader.name));
        created.fragment = createModule(device, variant.fragment, std::format("{} (fragment)", shader.name));
    }
}

bool GameShader::serves(contract::renderer::ShaderTarget target) const noexcept {
    return *_variants.at(static_cast<std::size_t>(target)).vertex != nullptr;
}

const pipeline::GraphicsPipeline* GameShader::getPipeline(contract::renderer::ShaderTarget target,
                                                          const core::Device& device,
                                                          const pipeline::PipelineLayout& layout,
                                                          vk::Format colorFormat, const vk::raii::PipelineCache* cache,
                                                          PipelineBuilder build) {
    Variant& variant = _variants.at(static_cast<std::size_t>(target));
    if (*variant.vertex == nullptr) {
        return nullptr;
    }
    if (!variant.built || variant.format != colorFormat) {
        variant.built.reset();
        variant.built.emplace(
            build(device, variant.vertex, variant.fragment, _instanceValues, layout, colorFormat, cache));
        variant.format = colorFormat;
        device.setName(*variant.built->getPipeline(),
                       std::format("{} {} pipeline", _name, kTargetNames.at(static_cast<std::size_t>(target))));
    }
    return &*variant.built;
}

}  // namespace rtype::render::vulkan::material
