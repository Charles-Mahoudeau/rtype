/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCache
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief Creates the shader modules of the renderer and keeps each one, so asking twice gives the same module.
///
/// @details Shaders come from two places:
/// - built-in shaders, compiled at build time and embedded in the binary by the glsl.spirv rule
///   (#include "sprite.vert.h" defines rtype_shader_sprite_vert): get("sprite.vert", rtype_shader_sprite_vert);
/// - shaders of the game, as SPIR-V files: load("enemy.frag.spv"). A relative path is resolved against the shader
///   directory given at construction (the renderer's: next to the executable by default, see
///   VulkanRenderer::Config::shaderDirectory), never against the working directory.
///
/// @warning Destroy it before the Device.
class RTYPE_RENDER_VULKAN_API ShaderCache {
  public:
    /// @param device The device the modules are created on; must outlive the cache.
    /// @param shaderDirectory Where load() looks for relative paths; should be absolute.
    ShaderCache(const core::Device& device, std::filesystem::path shaderDirectory);
    ~ShaderCache() = default;
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = delete;
    ShaderCache& operator=(ShaderCache&&) = delete;

    /// @return The directory load() resolves relative paths against.
    [[nodiscard]] const std::filesystem::path& getShaderDirectory() const noexcept { return _shaderDirectory; }

    /// @return The number of modules created so far.
    [[nodiscard]] std::size_t getModuleCount() const noexcept { return _builtins.size() + _files.size(); }

    /// @return The module of the built-in shader @p name, created from @p spirv the first time.
    /// @param name Its file name, e.g. "sprite.vert": the cache key.
    /// @param spirv Its embedded SPIR-V, e.g. rtype_shader_sprite_vert.
    /// @throws exceptions::ShaderException If @p spirv is not SPIR-V.
    /// @throws vk::SystemError If the module cannot be created.
    [[nodiscard]] const vk::raii::ShaderModule& get(std::string_view name, std::span<const std::uint32_t> spirv);

    /// @return The module of the SPIR-V file at @p path, read and created the first time.
    /// @param path Absolute, or relative to getShaderDirectory().
    /// @throws exceptions::ShaderException If the file is missing or not SPIR-V.
    /// @throws vk::SystemError If the module cannot be created.
    [[nodiscard]] const vk::raii::ShaderModule& load(const std::filesystem::path& path);

  private:
    /// @return A new module for @p spirv, which must already be validated.
    [[nodiscard]] vk::raii::ShaderModule createModule(std::span<const std::uint32_t> spirv) const;

    const core::Device* _device;                                        ///< Creates the modules; not owned.
    std::filesystem::path _shaderDirectory;                             ///< Base of relative load() paths.
    std::unordered_map<std::string, vk::raii::ShaderModule> _builtins;  ///< Built-in shaders, by name.
    std::unordered_map<std::string, vk::raii::ShaderModule> _files;     ///< Game shaders, by resolved path.
};

}  // namespace rtype::render::vulkan::pipeline
