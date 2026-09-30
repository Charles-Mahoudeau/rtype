/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Instance
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::vulkan::core {
class Instance {
  public:
    /// @param requiredExtensions Instance extensions to enable, e.g. the ones the window needs for its surface.
    /// Taken by value: VK_KHR_portability_enumeration is appended when the loader offers it (MoltenVK).
    /// @throws std::runtime_error If one of the required extensions is not supported.
    Instance(const std::string_view& appName, const std::string_view& engineName, uint32_t apiVersion,
             std::vector<const char*> requiredExtensions, bool enableValidationLayers = false);
    ~Instance() = default;
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&&) = delete;
    Instance& operator=(Instance&&) = delete;

  protected:
  private:
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
};
}  // namespace rtype::vulkan::core
