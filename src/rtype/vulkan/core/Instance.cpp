/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Instance
*/

#include "Instance.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace {
/// @return True if the loader offers the instance extension.
bool isExtensionAvailable(std::span<const vk::ExtensionProperties> available, std::string_view name) {
    return std::ranges::any_of(available, [name](const vk::ExtensionProperties& properties) {
        return std::string_view(properties.extensionName) == name;
    });
}
}  // namespace

namespace rtype::vulkan::core {
Instance::Instance(const std::string_view& appName, const std::string_view& engineName, uint32_t apiVersion,
                   std::vector<const char*> requiredExtensions, bool enableValidationLayers) {
    const uint32_t vkApplicationVersion = VK_MAKE_VERSION(1, 0, 0);
    const uint32_t vkEngineVersion = VK_MAKE_VERSION(1, 0, 0);

    const std::string applicationName(appName);
    const std::string engineNameString(engineName);
    const vk::ApplicationInfo appInfo(applicationName.c_str(), vkApplicationVersion, engineNameString.c_str(),
                                      vkEngineVersion, apiVersion);

    const auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (const char* requiredExtension : requiredExtensions) {
        if (!isExtensionAvailable(extensionProperties, requiredExtension)) {
            throw std::runtime_error("Required instance extension not supported: " + std::string(requiredExtension));
        }
    }

    vk::InstanceCreateFlags flags{};
    if (isExtensionAvailable(extensionProperties, vk::KHRPortabilityEnumerationExtensionName)) {
        requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
        flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }

    vk::InstanceCreateInfo createInfo{};
    createInfo.setFlags(flags).setPApplicationInfo(&appInfo).setPEnabledExtensionNames(requiredExtensions);

    instance = vk::raii::Instance(context, createInfo);
}

}  // namespace rtype::vulkan::core
