/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ObjectHandleTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "render/vulkan/core/Device.hpp"

using rtype::render::vulkan::core::objectHandleOf;

TEST(ObjectHandle, IsZeroForANullHandle) {
    EXPECT_EQ(objectHandleOf(vk::Buffer{}), 0U);
    EXPECT_EQ(objectHandleOf(vk::CommandBuffer{}), 0U);
}

TEST(ObjectHandle, KeepsTheValueOfAHandle) {
    constexpr std::uint64_t kValue = 0x1234;
    const vk::Buffer buffer{
        reinterpret_cast<VkBuffer>(kValue)};  // NOLINT(*-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    EXPECT_EQ(objectHandleOf(buffer), kValue);
}
