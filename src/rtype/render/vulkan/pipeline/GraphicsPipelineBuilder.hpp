/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsPipelineBuilder
*/

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief Builds graphics pipelines for dynamic rendering in a few lines, with defaults that suit 2D.
///
/// @details Defaults: triangle list, no culling (a sprite flipped by a negative scale stays visible), one sample, no
/// depth, opaque blending, viewport and scissor dynamic (set when recording, so a resize recreates no pipeline).
/// Only the shaders, the color format and the layout are required:
/// @code
/// const vk::raii::Pipeline pipeline = GraphicsPipelineBuilder{device}
///                                         .setShaders(vertex, fragment)
///                                         .setBlend(GraphicsPipelineBuilder::BlendMode::kAlpha)
///                                         .setColorFormat(swapchain.getFormat())
///                                         .setLayout(layout)
///                                         .build();
/// @endcode
/// The builder keeps references to the shaders, the specialization info and the layout: they must stay alive until
/// build() returns.
class RTYPE_RENDER_VULKAN_API GraphicsPipelineBuilder {
  public:
    /// @brief How the fragments are combined with what the color attachment already holds.
    enum class BlendMode : std::uint8_t {
        kOpaque,    ///< Replaces it.
        kAlpha,     ///< Blends by the fragment's alpha: sprites, text, transparent shapes.
        kAdditive,  ///< Adds to it, weighted by alpha: glows, explosions, lasers.
    };

    /// @param device The device the pipelines are created on; must outlive the builder.
    explicit GraphicsPipelineBuilder(const core::Device& device);
    ~GraphicsPipelineBuilder() = default;
    GraphicsPipelineBuilder(const GraphicsPipelineBuilder&) = default;
    GraphicsPipelineBuilder& operator=(const GraphicsPipelineBuilder&) = default;
    GraphicsPipelineBuilder(GraphicsPipelineBuilder&&) noexcept = default;
    GraphicsPipelineBuilder& operator=(GraphicsPipelineBuilder&&) noexcept = default;

    /// @brief Sets the vertex and fragment shaders, both with entry point "main". Required.
    /// @param specialization Specialization constants of both stages, or null.
    GraphicsPipelineBuilder& setShaders(const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
                                        const vk::SpecializationInfo* specialization = nullptr);
    /// @brief Sets the vertex buffer layout. Default: none, the vertex shader builds its vertices from
    /// gl_VertexIndex.
    GraphicsPipelineBuilder& setVertexInput(std::span<const vk::VertexInputBindingDescription> bindings,
                                            std::span<const vk::VertexInputAttributeDescription> attributes);
    /// @brief Sets the blending preset. Default: kOpaque.
    GraphicsPipelineBuilder& setBlend(BlendMode mode);
    /// @brief Sets which faces are discarded. Default: none.
    GraphicsPipelineBuilder& setCullMode(vk::CullModeFlags mode);
    /// @brief Sets the format of the color attachment rendered into, e.g. the swapchain's. Required.
    GraphicsPipelineBuilder& setColorFormat(vk::Format format);
    /// @brief Enables the depth test against a depth attachment of @p format, with writes if @p write. Default: no
    /// depth.
    GraphicsPipelineBuilder& setDepth(vk::Format format, bool write);
    /// @brief Sets the pipeline layout: descriptor set layouts and push constant ranges. Required.
    GraphicsPipelineBuilder& setLayout(const vk::raii::PipelineLayout& layout);

    /// @return A new pipeline with the settings so far.
    /// @param cache Shared pipeline cache to speed up creation, or null.
    /// @throws std::logic_error If the shaders, the color format or the layout are missing.
    /// @throws vk::SystemError If the pipeline cannot be created.
    [[nodiscard]] vk::raii::Pipeline build(const vk::raii::PipelineCache* cache = nullptr) const;

  private:
    const core::Device* _device;                                   ///< Creates the pipelines; not owned.
    const vk::raii::ShaderModule* _vertexShader = nullptr;         ///< Not owned.
    const vk::raii::ShaderModule* _fragmentShader = nullptr;       ///< Not owned.
    const vk::SpecializationInfo* _specialization = nullptr;       ///< Not owned; null when none.
    std::vector<vk::VertexInputBindingDescription> _bindings;      ///< Vertex buffer bindings.
    std::vector<vk::VertexInputAttributeDescription> _attributes;  ///< Vertex attributes.
    BlendMode _blendMode = BlendMode::kOpaque;                     ///< Blending preset.
    vk::CullModeFlags _cullMode = vk::CullModeFlagBits::eNone;     ///< Discarded faces.
    std::optional<vk::Format> _colorFormat;                        ///< Color attachment format.
    std::optional<vk::Format> _depthFormat;                        ///< Depth attachment format; empty: no depth.
    bool _depthWrite = false;                                      ///< Whether the depth test also writes.
    const vk::raii::PipelineLayout* _layout = nullptr;             ///< Not owned.
};

/// @return A pipeline layout made of @p setLayouts and @p pushConstants.
/// @throws vk::SystemError If it cannot be created.
[[nodiscard]] RTYPE_RENDER_VULKAN_API vk::raii::PipelineLayout createPipelineLayout(
    const core::Device& device, std::span<const vk::DescriptorSetLayout> setLayouts,
    std::span<const vk::PushConstantRange> pushConstants);

}  // namespace rtype::render::vulkan::pipeline
