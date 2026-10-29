/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleVertexInputTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Particle.hpp"
#include "render/vulkan/rendering/ParticleVertexInput.hpp"

using rtype::plugins::backend::graphics::ParticleInstance;

TEST(ParticleVertexInput, OneInstancePerParticle) {
    const auto bindings = rtype::render::vulkan::rendering::getParticleBindings(2);
    EXPECT_EQ(bindings.at(0).binding, 2U);
    EXPECT_EQ(bindings.at(0).stride, sizeof(ParticleInstance));
    EXPECT_EQ(bindings.at(0).inputRate, vk::VertexInputRate::eInstance);
}

TEST(ParticleVertexInput, ReadsTheFieldsParticleVertReads) {
    const auto attributes = rtype::render::vulkan::rendering::getParticleAttributes(2);
    EXPECT_EQ(attributes.at(0).location, 0U);
    EXPECT_EQ(attributes.at(0).offset, offsetof(ParticleInstance, color));
    EXPECT_EQ(attributes.at(0).format, vk::Format::eR32G32B32A32Sfloat);
    EXPECT_EQ(attributes.at(1).offset, offsetof(ParticleInstance, position));
    EXPECT_EQ(attributes.at(2).offset, offsetof(ParticleInstance, size));
    EXPECT_EQ(attributes.at(3).offset, offsetof(ParticleInstance, rotation));
    EXPECT_EQ(attributes.at(3).format, vk::Format::eR32Sfloat);
    for (const vk::VertexInputAttributeDescription& attribute : attributes) {
        EXPECT_EQ(attribute.binding, 2U);
    }
}
