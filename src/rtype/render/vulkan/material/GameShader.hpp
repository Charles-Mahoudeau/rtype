/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameShader
*/

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shader.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"

namespace rtype::render::vulkan::material {

/// @brief A game shader, behind a plugins::backend::graphics::ShaderId: the modules of each target it serves, and the
/// pipeline each one is drawn with, built the first time it is drawn.
///
/// @details A pipeline is built for one colour format: drawn into another one (a swapchain recreated in another
/// format), it is built again. The format only changes when the swapchain is recreated, after vkDeviceWaitIdle, so the
/// old pipeline is no longer in use and is destroyed at once.
/// @warning Destroy it before the Device.
class RTYPE_RENDER_VULKAN_API GameShader {
  public:
    /// @brief Builds the pipeline of a target: its modules, the colour format, and the layout and cache to use.
    using PipelineBuilder = pipeline::GraphicsPipeline (*)(
        const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
        const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache);

    /// @brief Creates the modules of every variant of @p shader.
    /// @throws std::invalid_argument If @p shader is invalid (plugins::backend::graphics::validate()).
    /// @throws exceptions::ShaderException If a stage is not SPIR-V.
    /// @throws vk::SystemError If a module cannot be created.
    GameShader(const core::Device& device, const plugins::backend::graphics::CompiledShader& shader);
    ~GameShader() = default;
    GameShader(const GameShader&) = delete;
    GameShader& operator=(const GameShader&) = delete;
    GameShader(GameShader&&) noexcept = default;
    GameShader& operator=(GameShader&&) noexcept = default;

    /// @return The shader's name, for messages.
    [[nodiscard]] const std::string& getName() const noexcept { return _name; }
    /// @return The bytes of its material block (0: none).
    [[nodiscard]] std::uint32_t getMaterialSize() const noexcept { return _materialSize; }

    /// @return Whether it serves @p target.
    [[nodiscard]] bool serves(plugins::backend::graphics::ShaderTarget target) const noexcept;

    /// @return The pipeline drawing @p target into @p colorFormat, built by @p build the first time (or after a format
    /// change), or nullptr if the shader does not serve @p target.
    /// @throws vk::SystemError If the pipeline cannot be built.
    [[nodiscard]] const pipeline::GraphicsPipeline* getPipeline(plugins::backend::graphics::ShaderTarget target,
                                                                const core::Device& device,
                                                                const pipeline::PipelineLayout& layout,
                                                                vk::Format colorFormat,
                                                                const vk::raii::PipelineCache* cache,
                                                                PipelineBuilder build);

  private:
    /// @brief What the shader draws one target with.
    struct Variant {
        vk::raii::ShaderModule vertex{nullptr};           ///< Null if the shader does not serve the target.
        vk::raii::ShaderModule fragment{nullptr};         ///< Null if the shader does not serve the target.
        std::optional<pipeline::GraphicsPipeline> built;  ///< Built for `format`, by the first draw.
        vk::Format format{vk::Format::eUndefined};        ///< The colour format `built` draws into.
    };

    std::string _name;                                                              ///< For messages.
    std::uint32_t _materialSize{0};                                                 ///< Its material block.
    std::array<Variant, plugins::backend::graphics::kShaderTargetCount> _variants;  ///< By ShaderTarget.
};

}  // namespace rtype::render::vulkan::material
