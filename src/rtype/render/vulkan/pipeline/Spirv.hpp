/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Spirv
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief First word of every SPIR-V module.
inline constexpr std::uint32_t kSpirvMagic = 0x07230203;

/// @brief Checks that @p code looks like a SPIR-V module: not empty, starting with kSpirvMagic.
/// @param code The SPIR-V words.
/// @param name What to call the module in the error message: its path or shader name.
/// @throws exceptions::ShaderException If it is not SPIR-V.
RTYPE_RENDER_VULKAN_API void validateSpirv(std::span<const std::uint32_t> code, std::string_view name);

/// @return The SPIR-V words of the file at @p path, checked by validateSpirv().
/// @throws exceptions::ShaderException If the file is missing or unreadable, its size is not a whole number of
/// 32-bit words, or it is not SPIR-V.
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::vector<std::uint32_t> readSpirv(const std::filesystem::path& path);

}  // namespace rtype::render::vulkan::pipeline
