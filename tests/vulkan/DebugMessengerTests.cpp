/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DebugMessengerTests
*/

#include <gtest/gtest.h>

#include <array>
#include <vulkan/vulkan.hpp>

#include "render/vulkan/core/DebugMessenger.hpp"

using rtype::render::vulkan::core::DebugMessenger;

TEST(DebugMessenger, DescribesNothingWithoutLabelsOrObjects) {
    EXPECT_TRUE(DebugMessenger::describeContext(vk::DebugUtilsMessengerCallbackDataEXT{}).empty());
}

TEST(DebugMessenger, DescribesTheLabelsThenTheNamedObjects) {
    const std::array labels{vk::DebugUtilsLabelEXT{}.setPLabelName("Sprites")};
    const std::array objects{vk::DebugUtilsObjectNameInfoEXT{}
                                 .setObjectType(vk::ObjectType::eCommandBuffer)
                                 .setPObjectName("Frame 0 command buffer")};
    const auto data = vk::DebugUtilsMessengerCallbackDataEXT{}.setCmdBufLabels(labels).setObjects(objects);
    EXPECT_EQ(DebugMessenger::describeContext(data), "\n  In: Sprites\n  CommandBuffer: Frame 0 command buffer");
}

TEST(DebugMessenger, DescribesAnUnnamedObjectByItsHandle) {
    const std::array objects{
        vk::DebugUtilsObjectNameInfoEXT{}.setObjectType(vk::ObjectType::eBuffer).setObjectHandle(0x2A)};
    const auto data = vk::DebugUtilsMessengerCallbackDataEXT{}.setObjects(objects);
    EXPECT_EQ(DebugMessenger::describeContext(data), "\n  Buffer: 0x2a");
}
