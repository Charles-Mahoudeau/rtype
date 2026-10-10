/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsPipelineBuilder
*/

#include "GraphicsPipelineBuilder.hpp"

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/rendering/DynamicRendering.hpp"

namespace rtype::render::vulkan::pipeline {

namespace {

/// @return The color blend state of @p mode, writing every channel (the default mask writes none).
vk::PipelineColorBlendAttachmentState blendAttachment(GraphicsPipelineBuilder::BlendMode mode) {
    using Factor = vk::BlendFactor;
    const auto state = vk::PipelineColorBlendAttachmentState{}
                           .setColorBlendOp(vk::BlendOp::eAdd)
                           .setAlphaBlendOp(vk::BlendOp::eAdd)
                           .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    switch (mode) {
        case GraphicsPipelineBuilder::BlendMode::kOpaque:
            return vk::PipelineColorBlendAttachmentState{state}.setBlendEnable(vk::False);
        case GraphicsPipelineBuilder::BlendMode::kAlpha:
            return vk::PipelineColorBlendAttachmentState{state}
                .setBlendEnable(vk::True)
                .setSrcColorBlendFactor(Factor::eSrcAlpha)
                .setDstColorBlendFactor(Factor::eOneMinusSrcAlpha)
                .setSrcAlphaBlendFactor(Factor::eOne)
                .setDstAlphaBlendFactor(Factor::eOneMinusSrcAlpha);
        case GraphicsPipelineBuilder::BlendMode::kAdditive:
            return vk::PipelineColorBlendAttachmentState{state}
                .setBlendEnable(vk::True)
                .setSrcColorBlendFactor(Factor::eSrcAlpha)
                .setDstColorBlendFactor(Factor::eOne)
                .setSrcAlphaBlendFactor(Factor::eZero)
                .setDstAlphaBlendFactor(Factor::eOne);
    }
    throw std::logic_error("GraphicsPipelineBuilder: invalid blend mode");
}

}  // namespace

GraphicsPipelineBuilder::GraphicsPipelineBuilder(const core::Device& device) : _device{&device} {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setShaders(const vk::raii::ShaderModule& vertex,
                                                             const vk::raii::ShaderModule& fragment,
                                                             const vk::SpecializationInfo* specialization) {
    _vertexShader = &vertex;
    _fragmentShader = &fragment;
    _specialization = specialization;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInput(
    std::span<const vk::VertexInputBindingDescription> bindings,
    std::span<const vk::VertexInputAttributeDescription> attributes) {
    _bindings.assign(bindings.begin(), bindings.end());
    _attributes.assign(attributes.begin(), attributes.end());
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setBlend(BlendMode mode) {
    _blendMode = mode;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setCullMode(vk::CullModeFlags mode) {
    _cullMode = mode;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setColorFormat(vk::Format format) {
    _colorFormat = format;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepth(vk::Format format, bool write) {
    _depthFormat = format;
    _depthWrite = write;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setLayout(const vk::raii::PipelineLayout& layout) {
    _layout = &layout;
    return *this;
}

vk::raii::Pipeline GraphicsPipelineBuilder::build(const vk::raii::PipelineCache* cache) const {
    if (_vertexShader == nullptr || _fragmentShader == nullptr || !_colorFormat || _layout == nullptr) {
        throw std::logic_error("GraphicsPipelineBuilder: shaders, color format and layout are required");
    }
    const std::array stages{
        vk::PipelineShaderStageCreateInfo{
            {}, vk::ShaderStageFlagBits::eVertex, **_vertexShader, "main", _specialization},
        vk::PipelineShaderStageCreateInfo{
            {}, vk::ShaderStageFlagBits::eFragment, **_fragmentShader, "main", _specialization},
    };
    const auto vertexInput =
        vk::PipelineVertexInputStateCreateInfo{}.setVertexBindingDescriptions(_bindings).setVertexAttributeDescriptions(
            _attributes);
    const auto inputAssembly =
        vk::PipelineInputAssemblyStateCreateInfo{}.setTopology(vk::PrimitiveTopology::eTriangleList);
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
                                                    .setLayout(**_layout);
    return vk::raii::Pipeline{_device->getDevice(), cache, info};
}

vk::raii::PipelineLayout createPipelineLayout(const core::Device& device,
                                              std::span<const vk::DescriptorSetLayout> setLayouts,
                                              std::span<const vk::PushConstantRange> pushConstants) {
    return vk::raii::PipelineLayout{
        device.getDevice(),
        vk::PipelineLayoutCreateInfo{}.setSetLayouts(setLayouts).setPushConstantRanges(pushConstants)};
}

}  // namespace rtype::render::vulkan::pipeline
