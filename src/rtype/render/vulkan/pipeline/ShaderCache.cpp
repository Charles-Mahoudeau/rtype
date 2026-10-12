/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCache
*/

#include "ShaderCache.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Spirv.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::pipeline {

ShaderCache::ShaderCache(const core::Device& device, std::filesystem::path shaderDirectory)
    : _device{&device}, _shaderDirectory{std::move(shaderDirectory)} {}

const vk::raii::ShaderModule& ShaderCache::get(std::string_view name, std::span<const std::uint32_t> spirv) {
    std::string key{name};
    if (const auto found = _builtins.find(key); found != _builtins.end()) {
        return found->second;
    }
    validateSpirv(spirv, name);
    return _builtins.emplace(std::move(key), createModule(spirv)).first->second;
}

const vk::raii::ShaderModule& ShaderCache::load(const std::filesystem::path& path) {
    std::string key = (_shaderDirectory / path).lexically_normal().string();
    if (const auto found = _files.find(key); found != _files.end()) {
        return found->second;
    }
    const std::vector<std::uint32_t> spirv = readSpirv(key);
    return _files.emplace(std::move(key), createModule(spirv)).first->second;
}

vk::raii::ShaderModule ShaderCache::createModule(std::span<const std::uint32_t> spirv) const {
    return vk::raii::ShaderModule{_device->getDevice(), vk::ShaderModuleCreateInfo{}.setCode(spirv)};
}

}  // namespace rtype::render::vulkan::pipeline
