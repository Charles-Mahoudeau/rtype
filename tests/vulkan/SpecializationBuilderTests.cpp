/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpecializationBuilderTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/pipeline/SpecializationBuilder.hpp"

namespace {

using rtype::render::vulkan::pipeline::SpecializationBuilder;

/// @return The value of type T stored at @p offset in @p builder's data.
template <class T>
T valueAt(const SpecializationBuilder& builder, std::uint32_t offset) {
    T value{};
    std::memcpy(&value, &builder.getData()[offset], sizeof(T));
    return value;
}

}  // namespace

TEST(SpecializationBuilder, PacksConstantsInOrder) {
    SpecializationBuilder builder;
    builder.add(0, true).add(1, 4U).add(2, -3).add(3, 0.5F);

    ASSERT_EQ(builder.getEntries().size(), 4U);
    EXPECT_EQ(builder.getData().size(), 16U);
    EXPECT_EQ(builder.getEntries()[2].constantID, 2U);
    EXPECT_EQ(builder.getEntries()[2].offset, 8U);
    EXPECT_EQ(builder.getEntries()[2].size, sizeof(std::int32_t));

    EXPECT_EQ(valueAt<vk::Bool32>(builder, builder.getEntries()[0].offset), vk::True);
    EXPECT_EQ(valueAt<std::uint32_t>(builder, builder.getEntries()[1].offset), 4U);
    EXPECT_EQ(valueAt<std::int32_t>(builder, builder.getEntries()[2].offset), -3);
    EXPECT_FLOAT_EQ(valueAt<float>(builder, builder.getEntries()[3].offset), 0.5F);
}

TEST(SpecializationBuilder, BoolsAreVkBool32) {
    SpecializationBuilder builder;
    builder.add(7, false);
    EXPECT_EQ(builder.getEntries()[0].size, sizeof(vk::Bool32));
    EXPECT_EQ(valueAt<vk::Bool32>(builder, 0), vk::False);
}

TEST(SpecializationBuilder, BuildPointsIntoTheBuilder) {
    SpecializationBuilder builder;
    builder.add(0, 1U).add(5, 2U);
    const vk::SpecializationInfo info = builder.build();
    EXPECT_EQ(info.mapEntryCount, 2U);
    EXPECT_EQ(info.pMapEntries, builder.getEntries().data());
    EXPECT_EQ(info.dataSize, builder.getData().size());
    EXPECT_EQ(info.pData, builder.getData().data());
}

TEST(SpecializationBuilder, RejectsTheSameConstantTwice) {
    SpecializationBuilder builder;
    builder.add(3, 1U);
    EXPECT_THROW(builder.add(3, true), std::invalid_argument);
}
