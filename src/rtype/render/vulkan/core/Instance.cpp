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

namespace rtype::render::vulkan::core {
Instance::Instance(const std::string_view& appName, const std::string_view& engineName, uint32_t apiVersion,
                   std::vector<const char*> requiredExtensions, std::vector<const char*> layers)
    : _context(getLoaderEntryPoint()) {
    const std::string applicationName(appName);
    const std::string engineNameString(engineName);
    const vk::ApplicationInfo appInfo(applicationName.c_str(), VK_MAKE_VERSION(1, 0, 0), engineNameString.c_str(),
                                      VK_MAKE_VERSION(1, 0, 0), apiVersion);

    const auto availableExtensions = _context.enumerateInstanceExtensionProperties();
    checkExtensionsSupported(availableExtensions, requiredExtensions);
    const vk::InstanceCreateFlags flags = enablePortability(availableExtensions, requiredExtensions);
    checkLayersSupported(_context.enumerateInstanceLayerProperties(), layers);

    _instance = createInstance(appInfo, flags, requiredExtensions, layers);
}

const vk::raii::Instance& Instance::getInstance() const { return _instance; }

const vk::raii::Context& Instance::getContext() const { return _context; }

PFN_vkGetInstanceProcAddr Instance::getLoaderEntryPoint() noexcept { return vkGetInstanceProcAddr; }

void Instance::checkExtensionsSupported(std::span<const vk::ExtensionProperties> available,
                                        std::span<const char* const> required) {
    for (const char* extension : required) {
        if (!isExtensionAvailable(available, extension)) {
            throw std::runtime_error("Required instance extension not supported: " + std::string(extension));
        }
    }
}

void Instance::checkLayersSupported(std::span<const vk::LayerProperties> available,
                                    std::span<const char* const> requested) {
    for (const char* layer : requested) {
        if (!isLayerAvailable(available, layer)) {
            throw std::runtime_error("Instance layer not supported: " + std::string(layer));
        }
    }
}

vk::InstanceCreateFlags Instance::enablePortability(std::span<const vk::ExtensionProperties> available,
                                                    std::vector<const char*>& extensions) {
    if (!isExtensionAvailable(available, vk::KHRPortabilityEnumerationExtensionName)) {
        return {};
    }
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    return vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
}

bool Instance::isExtensionAvailable(std::span<const vk::ExtensionProperties> available, std::string_view name) {
    return std::ranges::any_of(available, [name](const vk::ExtensionProperties& properties) {
        return std::string_view(properties.extensionName) == name;
    });
}

bool Instance::isLayerAvailable(std::span<const vk::LayerProperties> available, std::string_view name) {
    return std::ranges::any_of(available, [name](const vk::LayerProperties& properties) {
        return std::string_view(properties.layerName) == name;
    });
}

vk::raii::Instance Instance::createInstance(const vk::ApplicationInfo& appInfo, vk::InstanceCreateFlags flags,
                                            std::span<const char* const> extensions,
                                            std::span<const char* const> layers) const {
    vk::InstanceCreateInfo createInfo{};
    createInfo.setFlags(flags)
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames(extensions)
        .setPEnabledLayerNames(layers);
    return {_context, createInfo};
}

}  // namespace rtype::render::vulkan::core
