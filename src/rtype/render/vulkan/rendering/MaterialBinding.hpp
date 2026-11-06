/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialBinding
*/

#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief What a batch with a game material is drawn with.
struct MaterialBinding {
    const pipeline::GraphicsPipeline* pipeline = nullptr;  ///< Its shader's; null: drawn as the default material.
    vk::DescriptorSet set;                                 ///< Its set 2, for this frame.
    bool instanceValues = false;                           ///< Its pipeline reads the instance values binding.
};

/// @brief Finds what a game material is drawn with this frame for a target, or a null pipeline if it cannot be drawn.
using MaterialFinder = std::function<MaterialBinding(contract::renderer::MaterialId, contract::renderer::ShaderTarget)>;

}  // namespace rtype::render::vulkan::rendering
