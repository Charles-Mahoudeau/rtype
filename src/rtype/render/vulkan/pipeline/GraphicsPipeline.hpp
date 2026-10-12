/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsPipeline
*/

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "PipelineLayout.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief A graphics pipeline for dynamic rendering, with the layout it was built for. Built by
/// GraphicsPipeline::Builder.
/// @warning Destroy it before the Device. Its PipelineLayout must outlive it.
class RTYPE_RENDER_VULKAN_API GraphicsPipeline {
  public:
    /// @brief How the fragments are combined with what the color attachment already holds.
    enum class BlendMode : std::uint8_t {
        kOpaque,    ///< Replaces it.
        kAlpha,     ///< Blends by the fragment's alpha: transparent shapes, sprites, text.
        kAdditive,  ///< Adds to it, weighted by alpha: glows, explosions, lasers.
    };

    /// @brief Builds graphics pipelines in a few lines, with defaults that suit 2D.
    ///
    /// @details Defaults: no vertex buffer, triangle list, no culling (a shape flipped by a negative scale stays
    /// visible), one sample, no depth, opaque blending, viewport and scissor dynamic (set when recording, so a resize
    /// recreates no pipeline). Only the shaders, the color format and the layout are required:
    /// @code
    /// const GraphicsPipeline pipeline = GraphicsPipeline::Builder{}
    ///                                       .setShaders(vertex, fragment)
    ///                                       .setBlend(GraphicsPipeline::BlendMode::kAlpha)
    ///                                       .setColorFormat(swapchain.getFormat())
    ///                                       .setLayout(layout)
    ///                                       .build(device);
    /// @endcode
    /// The builder keeps references to the shaders, the specialization info and the layout: they must stay alive
    /// until build() returns (and the layout, as long as the pipeline).
    class RTYPE_RENDER_VULKAN_API Builder {
      public:
        Builder() = default;
        ~Builder() = default;
        Builder(const Builder&) = default;
        Builder& operator=(const Builder&) = default;
        Builder(Builder&&) noexcept = default;
        Builder& operator=(Builder&&) noexcept = default;

        /// @brief Sets the vertex and fragment shaders, both with entry point "main". Required.
        /// @param specialization Specialization constants of both stages, or null.
        Builder& setShaders(const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
                            const vk::SpecializationInfo* specialization = nullptr);
        /// @brief Sets the vertex buffer layout. Default: none, the vertex shader builds its vertices (e.g. from
        /// gl_VertexIndex).
        Builder& setVertexInput(std::span<const vk::VertexInputBindingDescription> bindings,
                                std::span<const vk::VertexInputAttributeDescription> attributes);
        /// @brief Sets the primitive topology. Default: triangle list.
        Builder& setTopology(vk::PrimitiveTopology topology);
        /// @brief Sets the blending preset. Default: kOpaque.
        Builder& setBlend(BlendMode mode);
        /// @brief Sets which faces are discarded. Default: none.
        Builder& setCullMode(vk::CullModeFlags mode);
        /// @brief Sets the format of the color attachment rendered into, e.g. the swapchain's. Required.
        Builder& setColorFormat(vk::Format format);
        /// @brief Enables the depth test against a depth attachment of @p format, writing depth if @p write.
        /// Default: no depth.
        Builder& setDepth(vk::Format format, bool write);
        /// @brief Sets the layout: descriptor set layouts and push constant ranges. Required.
        Builder& setLayout(const PipelineLayout& layout);

        /// @return Whether the required settings (shaders, color format, layout) are set: build() can be called.
        [[nodiscard]] bool isComplete() const noexcept;

        /// @return A new pipeline with the settings so far.
        /// @param device The device the pipeline is created on.
        /// @param cache Shared pipeline cache to speed up creation, or null.
        /// @throws std::logic_error If the builder is not complete (isComplete()).
        /// @throws vk::SystemError If the pipeline cannot be created.
        [[nodiscard]] GraphicsPipeline build(const core::Device& device,
                                             const vk::raii::PipelineCache* cache = nullptr) const;

      private:
        const vk::raii::ShaderModule* _vertexShader = nullptr;                   ///< Not owned.
        const vk::raii::ShaderModule* _fragmentShader = nullptr;                 ///< Not owned.
        const vk::SpecializationInfo* _specialization = nullptr;                 ///< Not owned; null: none.
        std::vector<vk::VertexInputBindingDescription> _bindings;                ///< Vertex buffer bindings.
        std::vector<vk::VertexInputAttributeDescription> _attributes;            ///< Vertex attributes.
        vk::PrimitiveTopology _topology = vk::PrimitiveTopology::eTriangleList;  ///< How vertices form shapes.
        BlendMode _blendMode = BlendMode::kOpaque;                               ///< Blending preset.
        vk::CullModeFlags _cullMode = vk::CullModeFlagBits::eNone;               ///< Discarded faces.
        std::optional<vk::Format> _colorFormat;                                  ///< Color attachment format.
        std::optional<vk::Format> _depthFormat;                                  ///< Depth format; empty: none.
        bool _depthWrite = false;                                                ///< Whether depth is written.
        const PipelineLayout* _layout = nullptr;                                 ///< Not owned.
    };

    ~GraphicsPipeline() = default;
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&&) noexcept = default;
    GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default;

    /// @return The Vulkan pipeline.
    [[nodiscard]] const vk::raii::Pipeline& getPipeline() const noexcept { return _pipeline; }
    /// @return The layout it was built with, to bind descriptor sets and push constants.
    [[nodiscard]] const PipelineLayout& getLayout() const noexcept { return *_layout; }

    /// @brief Records the binding of this pipeline into @p commandBuffer.
    void bind(const vk::raii::CommandBuffer& commandBuffer) const;

  private:
    /// @brief Takes the pipeline built by a Builder.
    GraphicsPipeline(vk::raii::Pipeline pipeline, const PipelineLayout& layout);

    vk::raii::Pipeline _pipeline;   ///< Destroyed on destruction.
    const PipelineLayout* _layout;  ///< Not owned.
};

}  // namespace rtype::render::vulkan::pipeline
