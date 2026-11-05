/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DebugMessenger
*/

#include "DebugMessenger.hpp"

#include <array>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <vulkan/vulkan_raii.hpp>

#include "Instance.hpp"

namespace rtype::render::vulkan::core {

DebugMessenger::DebugMessenger(const Instance& instance, vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity)
    : _messenger{instance.getInstance(), makeCreateInfo(minSeverity)} {}

vk::DebugUtilsMessengerCreateInfoEXT DebugMessenger::makeCreateInfo(
    vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity) {
    return vk::DebugUtilsMessengerCreateInfoEXT{}
        .setMessageSeverity(severitiesFrom(minSeverity))
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        .setPfnUserCallback(&DebugMessenger::onMessage);
}

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

std::string DebugMessenger::describeContext(const vk::DebugUtilsMessengerCallbackDataEXT& data) {
    std::string context;
    for (const vk::DebugUtilsLabelEXT& label : std::span{data.pCmdBufLabels, data.cmdBufLabelCount}) {
        context += std::format("\n  In: {}", label.pLabelName);
    }
    for (const vk::DebugUtilsObjectNameInfoEXT& object : std::span{data.pObjects, data.objectCount}) {
        if (object.pObjectName != nullptr) {
            context += std::format("\n  {}: {}", vk::to_string(object.objectType), object.pObjectName);
        } else {
            context += std::format("\n  {}: {:#x}", vk::to_string(object.objectType), object.objectHandle);
        }
    }
    return context;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessenger::onMessage(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                           vk::DebugUtilsMessageTypeFlagsEXT types,
                                                           const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                                           void* /*userData*/) {
    std::cerr << "[Vulkan67 " << vk::to_string(severity) << "] " << vk::to_string(types) << " " << data->pMessage
              << describeContext(*data) << "\n";
    return vk::False;
}

}  // namespace rtype::render::vulkan::core
