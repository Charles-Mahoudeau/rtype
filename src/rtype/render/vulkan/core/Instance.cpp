/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Instance
*/

#include "Instance.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::core {
Instance::Instance(const std::string_view& appName, const std::string_view& engineName, uint32_t apiVersion,
                   std::vector<const char*> requiredExtensions, std::vector<const char*> layers,
                   ValidationOptions validation, const std::optional<vk::DebugUtilsMessengerCreateInfoEXT>& messenger)
    : _context{_loader.getEntryPoint()} {
    if (messenger && std::ranges::none_of(requiredExtensions, [](const char* extension) {
            return std::string_view{extension} == vk::EXTDebugUtilsExtensionName;
        })) {
        throw std::invalid_argument(std::format("Instance: a debug messenger needs {} in the required extensions",
                                                vk::EXTDebugUtilsExtensionName));
    }
    const std::string applicationName{appName};
    const std::string engineNameString{engineName};
    const vk::ApplicationInfo appInfo{applicationName.c_str(), VK_MAKE_VERSION(1, 0, 0), engineNameString.c_str(),
                                      VK_MAKE_VERSION(1, 0, 0), apiVersion};

    const auto availableExtensions = _context.enumerateInstanceExtensionProperties();
    checkExtensionsSupported(availableExtensions, requiredExtensions);
    const vk::InstanceCreateFlags flags = enablePortability(availableExtensions, requiredExtensions);
    layers = keepAvailableLayers(_context.enumerateInstanceLayerProperties(), layers);
    const ValidationOptions appliedValidation = enableValidationFeatures(validation, layers, requiredExtensions);

    _instance = createInstance(appInfo, flags, requiredExtensions, layers, appliedValidation, messenger);
}

const vk::raii::Instance& Instance::getInstance() const { return _instance; }

const vk::raii::Context& Instance::getContext() const { return _context; }

void Instance::checkExtensionsSupported(std::span<const vk::ExtensionProperties> available,
                                        std::span<const char* const> required) {
    for (const char* extension : required) {
        if (!isExtensionAvailable(available, extension)) {
            throw std::runtime_error(std::format("Required instance extension not supported: {}", extension));
        }
    }
}

std::vector<const char*> Instance::keepAvailableLayers(std::span<const vk::LayerProperties> available,
                                                       std::span<const char* const> requested) {
    std::vector<const char*> kept;
    for (const char* layer : requested) {
        if (isLayerAvailable(available, layer)) {
            kept.push_back(layer);
        } else {
            std::cerr << std::format("[vulkan] warning: the layer {} is not installed: running without it\n", layer);
        }
    }
    return kept;
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

ValidationOptions Instance::enableValidationFeatures(ValidationOptions requested, std::span<const char* const> layers,
                                                     std::vector<const char*>& extensions) const {
    const bool validationLayerEnabled =
        std::ranges::any_of(layers, [](const char* layer) { return std::string_view{layer} == kValidationLayerName; });
    if (!validationLayerEnabled || (!requested.synchronization && !requested.bestPractices)) {
        return {};
    }
    if (!isExtensionAvailable(_context.enumerateInstanceExtensionProperties(std::string{kValidationLayerName}),
                              vk::EXTLayerSettingsExtensionName)) {
        std::cerr << std::format(
            "[vulkan] warning: {} does not offer {}: running without synchronization "
            "validation and best practices\n",
            kValidationLayerName, vk::EXTLayerSettingsExtensionName);
        return {};
    }
    extensions.push_back(vk::EXTLayerSettingsExtensionName);
    return requested;
}

vk::raii::Instance Instance::createInstance(
    const vk::ApplicationInfo& appInfo, vk::InstanceCreateFlags flags, std::span<const char* const> extensions,
    std::span<const char* const> layers, ValidationOptions validation,
    const std::optional<vk::DebugUtilsMessengerCreateInfoEXT>& messenger) const {
    static constexpr vk::Bool32 kEnabled = vk::True;
    std::vector<vk::LayerSettingEXT> settings;
    if (validation.synchronization) {
        settings.emplace_back(kValidationLayerName, "validate_sync", vk::LayerSettingTypeEXT::eBool32, 1, &kEnabled);
    }
    if (validation.bestPractices) {
        settings.emplace_back(kValidationLayerName, "validate_best_practices", vk::LayerSettingTypeEXT::eBool32, 1,
                              &kEnabled);
    }
    const vk::LayerSettingsCreateInfoEXT layerSettings = vk::LayerSettingsCreateInfoEXT{}.setSettings(settings);

    const void* next = settings.empty() ? nullptr : &layerSettings;
    vk::DebugUtilsMessengerCreateInfoEXT messengerInfo{};
    if (messenger) {
        messengerInfo = *messenger;
        messengerInfo.setPNext(next);
        next = &messengerInfo;
    }

    vk::InstanceCreateInfo createInfo{};
    createInfo.setFlags(flags)
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames(extensions)
        .setPEnabledLayerNames(layers)
        .setPNext(next);
    try {
        return {_context, createInfo};
    } catch (const vk::LayerNotPresentError& error) {
        if (layers.empty()) {
            throw;
        }
        // The loader lists the layer, but cannot open its library (a manifest without its file): start without layers.
        std::cerr << std::format("[vulkan] warning: a layer could not be loaded ({}): running without layers\n",
                                 error.what());
    }
    std::vector<const char*> withoutSettings;
    for (const char* extension : extensions) {
        if (std::string_view{extension} != vk::EXTLayerSettingsExtensionName) {
            withoutSettings.push_back(extension);
        }
    }
    vk::DebugUtilsMessengerCreateInfoEXT plainMessenger{};
    const void* plainNext = nullptr;
    if (messenger) {
        plainMessenger = *messenger;
        plainMessenger.setPNext(nullptr);
        plainNext = &plainMessenger;
    }
    createInfo.setPEnabledExtensionNames(withoutSettings).setPEnabledLayerNames({}).setPNext(plainNext);
    return {_context, createInfo};
}

}  // namespace rtype::render::vulkan::core
