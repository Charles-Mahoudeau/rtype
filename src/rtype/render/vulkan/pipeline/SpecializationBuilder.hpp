/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpecializationBuilder
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief Builds the specialization constants of a shader stage: compile-time variants of one shader (e.g.
/// USE_TEXTURE, MAX_LIGHTS) without extra shader files.
///
/// @details In GLSL: layout(constant_id = 0) const bool USE_TEXTURE = false;
/// @code
/// SpecializationBuilder specialization;
/// specialization.add(0, true).add(1, 4U);               // USE_TEXTURE, MAX_LIGHTS
/// const vk::SpecializationInfo info = specialization.build();
/// // stage.setPSpecializationInfo(&info), then create the pipeline
/// @endcode
/// build() points into the builder: keep it, and the result, alive until the pipeline is created.
class RTYPE_RENDER_VULKAN_API SpecializationBuilder {
  public:
    SpecializationBuilder() = default;
    ~SpecializationBuilder() = default;
    SpecializationBuilder(const SpecializationBuilder&) = default;
    SpecializationBuilder& operator=(const SpecializationBuilder&) = default;
    SpecializationBuilder(SpecializationBuilder&&) noexcept = default;
    SpecializationBuilder& operator=(SpecializationBuilder&&) noexcept = default;

    /// @return The entries added so far: constant id, offset and size in getData().
    [[nodiscard]] std::span<const vk::SpecializationMapEntry> getEntries() const noexcept { return _entries; }
    /// @return The values added so far, packed one after the other.
    [[nodiscard]] std::span<const std::byte> getData() const noexcept { return _data; }

    /// @name Sets constant @p constantId; each id may be set once.
    /// @throws std::invalid_argument If @p constantId was already set.
    /// @{
    SpecializationBuilder& add(std::uint32_t constantId, bool value);  ///< As a VkBool32, the size of a GLSL bool.
    SpecializationBuilder& add(std::uint32_t constantId, std::int32_t value);
    SpecializationBuilder& add(std::uint32_t constantId, std::uint32_t value);
    SpecializationBuilder& add(std::uint32_t constantId, float value);
    /// @}

    /// @return The vk::SpecializationInfo of the constants added so far, pointing into this builder.
    [[nodiscard]] vk::SpecializationInfo build() const noexcept;

  private:
    /// @brief Appends the @p size bytes at @p value as constant @p constantId.
    /// @throws std::invalid_argument If @p constantId was already set.
    void append(std::uint32_t constantId, const void* value, std::size_t size);

    std::vector<vk::SpecializationMapEntry> _entries;  ///< One per constant.
    std::vector<std::byte> _data;                      ///< The values, at the entries' offsets.
};

}  // namespace rtype::render::vulkan::pipeline
