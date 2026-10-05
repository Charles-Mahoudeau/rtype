/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DebugMessenger
*/

#include "DebugMessenger.hpp"

#include <array>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

#include "Instance.hpp"

namespace rtype::render::vulkan::core {

DebugMessenger::DebugMessenger(const Instance& instance, vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity)
    : _messenger(instance.getInstance(), vk::DebugUtilsMessengerCreateInfoEXT{}
                                             .setMessageSeverity(severitiesFrom(minSeverity))
                                             .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                             vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                             vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
                                             .setPfnUserCallback(&DebugMessenger::onMessage)) {}

vk::DebugUtilsMessageSeverityFlagsEXT DebugMessenger::severitiesFrom(
    vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity) {
    constexpr std::array kSeverities = {
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose, vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning, vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};

    vk::DebugUtilsMessageSeverityFlagsEXT severities{};
    for (const auto severity : kSeverities) {
        if (severity >= minSeverity) {
            severities |= severity;
        }
    }
    return severities;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessenger::onMessage(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                           vk::DebugUtilsMessageTypeFlagsEXT types,
                                                           const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                                           void* /*userData*/) {
    std::cerr << "[Vulkan67 " << vk::to_string(severity) << "] " << vk::to_string(types) << " " << data->pMessage
              << "\n";
    return vk::False;
}

}  // namespace rtype::render::vulkan::core
