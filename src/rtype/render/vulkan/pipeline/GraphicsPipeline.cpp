/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsPipeline
*/

#include "GraphicsPipeline.hpp"

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include "PipelineLayout.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/rendering/DynamicRendering.hpp"

namespace rtype::render::vulkan::pipeline {

namespace {

/// @return The color blend state of @p mode, writing every channel (the default mask writes none).
vk::PipelineColorBlendAttachmentState blendAttachment(GraphicsPipeline::BlendMode mode) {
    using Factor = vk::BlendFactor;
    const auto state = vk::PipelineColorBlendAttachmentState{}
                           .setColorBlendOp(vk::BlendOp::eAdd)
                           .setAlphaBlendOp(vk::BlendOp::eAdd)
                           .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    switch (mode) {
        case GraphicsPipeline::BlendMode::kOpaque:
            return vk::PipelineColorBlendAttachmentState{state}.setBlendEnable(vk::False);
        case GraphicsPipeline::BlendMode::kAlpha:
            return vk::PipelineColorBlendAttachmentState{state}
                .setBlendEnable(vk::True)
                .setSrcColorBlendFactor(Factor::eSrcAlpha)
                .setDstColorBlendFactor(Factor::eOneMinusSrcAlpha)
                .setSrcAlphaBlendFactor(Factor::eOne)
                .setDstAlphaBlendFactor(Factor::eOneMinusSrcAlpha);
        case GraphicsPipeline::BlendMode::kAdditive:
            return vk::PipelineColorBlendAttachmentState{state}
                .setBlendEnable(vk::True)
                .setSrcColorBlendFactor(Factor::eSrcAlpha)
                .setDstColorBlendFactor(Factor::eOne)
                .setSrcAlphaBlendFactor(Factor::eZero)
                .setDstAlphaBlendFactor(Factor::eOne);
    }
    throw std::logic_error("GraphicsPipeline: invalid blend mode");
}

}  // namespace

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setShaders(const vk::raii::ShaderModule& vertex,
                                                                 const vk::raii::ShaderModule& fragment,
                                                                 const vk::SpecializationInfo* specialization) {
    _vertexShader = &vertex;
    _fragmentShader = &fragment;
    _specialization = specialization;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setVertexInput(
    std::span<const vk::VertexInputBindingDescription> bindings,
    std::span<const vk::VertexInputAttributeDescription> attributes) {
    _bindings.assign(bindings.begin(), bindings.end());
    _attributes.assign(attributes.begin(), attributes.end());
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setTopology(vk::PrimitiveTopology topology) {
    _topology = topology;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setBlend(BlendMode mode) {
    _blendMode = mode;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setCullMode(vk::CullModeFlags mode) {
    _cullMode = mode;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setColorFormat(vk::Format format) {
    _colorFormat = format;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setDepth(vk::Format format, bool write) {
    _depthFormat = format;
    _depthWrite = write;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::setLayout(const PipelineLayout& layout) {
    _layout = &layout;
    return *this;
}

bool GraphicsPipeline::Builder::isComplete() const noexcept {
    return _vertexShader != nullptr && _fragmentShader != nullptr && _colorFormat && _layout != nullptr;
}

GraphicsPipeline GraphicsPipeline::Builder::build(const core::Device& device,
                                                  const vk::raii::PipelineCache* cache) const {
    if (!_colorFormat || !isComplete()) {
        throw std::logic_error("GraphicsPipeline::Builder: the shaders, the color format and the layout are required");
    }
    // Everything the create info points to lives in this scope, until the pipeline is created.
    const std::array stages{
        vk::PipelineShaderStageCreateInfo{
            {}, vk::ShaderStageFlagBits::eVertex, **_vertexShader, "main", _specialization},
        vk::PipelineShaderStageCreateInfo{
            {}, vk::ShaderStageFlagBits::eFragment, **_fragmentShader, "main", _specialization},
    };
    const auto vertexInput =
        vk::PipelineVertexInputStateCreateInfo{}.setVertexBindingDescriptions(_bindings).setVertexAttributeDescriptions(
            _attributes);
    const auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{}.setTopology(_topology);
    const auto viewport = vk::PipelineViewportStateCreateInfo{}.setViewportCount(1).setScissorCount(1);
    const auto rasterization = vk::PipelineRasterizationStateCreateInfo{}.setCullMode(_cullMode).setLineWidth(1.0F);
    const auto multisample =
        vk::PipelineMultisampleStateCreateInfo{}.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    const auto depth = vk::PipelineDepthStencilStateCreateInfo{}
                           .setDepthTestEnable(_depthFormat ? vk::True : vk::False)
                           .setDepthWriteEnable(_depthWrite ? vk::True : vk::False)
                           .setDepthCompareOp(vk::CompareOp::eLessOrEqual);
    const vk::PipelineColorBlendAttachmentState attachment = blendAttachment(_blendMode);
    const auto blend = vk::PipelineColorBlendStateCreateInfo{}.setAttachments(attachment);
    const std::array dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    const auto dynamic = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStates);
    const std::array colorFormats{*_colorFormat};
    const vk::PipelineRenderingCreateInfo rendering =
        rendering::pipelineRenderingInfo(colorFormats, _depthFormat.value_or(vk::Format::eUndefined));

    const vk::GraphicsPipelineCreateInfo info = vk::GraphicsPipelineCreateInfo{}
                                                    .setPNext(&rendering)
                                                    .setStages(stages)
                                                    .setPVertexInputState(&vertexInput)
                                                    .setPInputAssemblyState(&inputAssembly)
                                                    .setPViewportState(&viewport)
                                                    .setPRasterizationState(&rasterization)
                                                    .setPMultisampleState(&multisample)
                                                    .setPDepthStencilState(&depth)
                                                    .setPColorBlendState(&blend)
                                                    .setPDynamicState(&dynamic)
                                                    .setLayout(*_layout->getLayout());
    return GraphicsPipeline{vk::raii::Pipeline{device.getDevice(), cache, info}, *_layout};
}

GraphicsPipeline::GraphicsPipeline(vk::raii::Pipeline pipeline, const PipelineLayout& layout)
    : _pipeline{std::move(pipeline)}, _layout{&layout} {}

void GraphicsPipeline::bind(const vk::raii::CommandBuffer& commandBuffer) const {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
}

}  // namespace rtype::render::vulkan::pipeline
