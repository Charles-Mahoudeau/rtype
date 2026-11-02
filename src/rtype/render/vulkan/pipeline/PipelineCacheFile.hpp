/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineCacheFile
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

/// @brief Saves a PipelineCache's data to disk and reads it back at the next launch, so pipelines are not compiled
/// again (issue #47).
namespace rtype::render::vulkan::pipeline {

/// @return Whether @p data starts with the header of a pipeline cache made by the device of @p properties
/// (VkPipelineCacheHeaderVersionOne: its vendor, its device and its pipelineCacheUUID, which changes with the driver).
/// Some drivers do not check it themselves, and crash on data from another GPU or driver.
[[nodiscard]] RTYPE_RENDER_VULKAN_API bool isCompatiblePipelineCache(
    std::span<const std::byte> data, const vk::PhysicalDeviceProperties& properties) noexcept;

/// @return The content of the pipeline cache file at @p path, or nothing if it does not exist or cannot be read.
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::vector<std::byte> readPipelineCacheFile(const std::filesystem::path& path);

/// @brief Writes @p data to the pipeline cache file at @p path, creating its directory: into a temporary file next
/// to it first, then renamed over it, so a crash while writing never leaves a truncated cache behind.
/// @throws std::runtime_error If the file cannot be written.
RTYPE_RENDER_VULKAN_API void writePipelineCacheFile(const std::filesystem::path& path,
                                                    std::span<const std::uint8_t> data);

}  // namespace rtype::render::vulkan::pipeline
