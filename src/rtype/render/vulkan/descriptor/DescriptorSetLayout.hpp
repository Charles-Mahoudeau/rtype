/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DescriptorSetLayout
*/

#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::descriptor {

/// @brief The bindings of one descriptor set (in GLSL: layout(set = N, binding = M)), built by
/// DescriptorSetLayout::Builder. Needed to create a PipelineLayout and to allocate sets from a DescriptorPool.
/// @warning Destroy it before the Device.
class RTYPE_RENDER_VULKAN_API DescriptorSetLayout {
  public:
    /// @brief Collects the bindings of a set, then creates its layout:
    /// @code
    /// const DescriptorSetLayout layout = DescriptorSetLayout::Builder{}
    ///                                        .addBinding(0, vk::DescriptorType::eUniformBuffer,
    ///                                                    vk::ShaderStageFlagBits::eVertex)
    ///                                        .addBinding(1, vk::DescriptorType::eCombinedImageSampler,
    ///                                                    vk::ShaderStageFlagBits::eFragment)
    ///                                        .build(device);
    /// @endcode
    class RTYPE_RENDER_VULKAN_API Builder {
      public:
        Builder() = default;
        ~Builder() = default;
        Builder(const Builder&) = default;
        Builder& operator=(const Builder&) = default;
        Builder(Builder&&) noexcept = default;
        Builder& operator=(Builder&&) noexcept = default;

        /// @return The bindings added so far.
        [[nodiscard]] std::span<const vk::DescriptorSetLayoutBinding> getBindings() const noexcept { return _bindings; }

        /// @brief Adds binding @p binding: @p count descriptors of @p type, read by @p stages.
        /// @throws std::invalid_argument If @p binding was already added, or @p count is 0.
        Builder& addBinding(std::uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages,
                            std::uint32_t count = 1);

        /// @return A new layout with the bindings added so far.
        /// @throws vk::SystemError If it cannot be created.
        [[nodiscard]] DescriptorSetLayout build(const core::Device& device) const;

      private:
        std::vector<vk::DescriptorSetLayoutBinding> _bindings;  ///< In the order they were added.
    };

    ~DescriptorSetLayout() = default;
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&&) noexcept = default;
    DescriptorSetLayout& operator=(DescriptorSetLayout&&) noexcept = default;

    /// @return The Vulkan layout.
    [[nodiscard]] const vk::raii::DescriptorSetLayout& getLayout() const noexcept { return _layout; }
    /// @return Its bindings.
    [[nodiscard]] std::span<const vk::DescriptorSetLayoutBinding> getBindings() const noexcept { return _bindings; }

  private:
    /// @brief Takes the layout created by a Builder, with its bindings.
    DescriptorSetLayout(vk::raii::DescriptorSetLayout layout, std::vector<vk::DescriptorSetLayoutBinding> bindings);

    vk::raii::DescriptorSetLayout _layout;                  ///< Destroyed on destruction.
    std::vector<vk::DescriptorSetLayoutBinding> _bindings;  ///< Copy of the bindings.
};

}  // namespace rtype::render::vulkan::descriptor
