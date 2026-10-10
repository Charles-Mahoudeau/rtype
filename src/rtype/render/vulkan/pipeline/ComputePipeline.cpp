/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComputePipeline
*/

#include "ComputePipeline.hpp"

#include <vulkan/vulkan_raii.hpp>

#include "PipelineLayout.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

ComputePipeline::ComputePipeline(const core::Device& device, const vk::raii::ShaderModule& shader,
                                 const PipelineLayout& layout, const vk::SpecializationInfo* specialization,
                                 const vk::raii::PipelineCache* cache)
    : _pipeline{device.getDevice(), cache,
                vk::ComputePipelineCreateInfo{}
                    .setStage(vk::PipelineShaderStageCreateInfo{
                        {}, vk::ShaderStageFlagBits::eCompute, *shader, "main", specialization})
                    .setLayout(*layout.getLayout())},
      _layout{&layout} {}

void ComputePipeline::bind(const vk::raii::CommandBuffer& commandBuffer) const {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *_pipeline);
}

}  // namespace rtype::render::vulkan::pipeline
