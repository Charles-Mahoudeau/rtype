/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameShader
*/

#include "GameShader.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shader.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/Spirv.hpp"

namespace rtype::render::vulkan::material {

namespace {

/// @return A module for @p spirv, checked first.
vk::raii::ShaderModule createModule(const core::Device& device, std::span<const std::uint32_t> spirv,
                                    std::string_view name) {
    pipeline::validateSpirv(spirv, name);
    return vk::raii::ShaderModule{device.getDevice(), vk::ShaderModuleCreateInfo{{}, spirv}};
}

}  // namespace

GameShader::GameShader(const core::Device& device, const plugins::backend::graphics::CompiledShader& shader)
    : _name{shader.name}, _materialSize{shader.materialSize} {
    plugins::backend::graphics::validate(shader);
    for (const plugins::backend::graphics::ShaderVariant& variant : shader.variants) {
        Variant& created = _variants.at(static_cast<std::size_t>(variant.target));
        created.vertex = createModule(device, variant.vertex, std::format("{} (vertex)", shader.name));
        created.fragment = createModule(device, variant.fragment, std::format("{} (fragment)", shader.name));
    }
}

bool GameShader::serves(plugins::backend::graphics::ShaderTarget target) const noexcept {
    return *_variants.at(static_cast<std::size_t>(target)).vertex != nullptr;
}

const pipeline::GraphicsPipeline* GameShader::getPipeline(plugins::backend::graphics::ShaderTarget target,
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
        variant.built.emplace(build(device, variant.vertex, variant.fragment, layout, colorFormat, cache));
        variant.format = colorFormat;
    }
    return &*variant.built;
}

}  // namespace rtype::render::vulkan::material
