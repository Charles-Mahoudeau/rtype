/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PremultiplyTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "render/vulkan/texture/Premultiply.hpp"

namespace {
/// @return One RGBA8 pixel.
std::array<std::byte, 4> pixel(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
    return {std::byte{red}, std::byte{green}, std::byte{blue}, std::byte{alpha}};
}
}  // namespace

TEST(PremultiplySrgb, LeavesOpaquePixelsUnchanged) {
    const auto opaque = pixel(12, 128, 250, 255);
    const std::vector<std::byte> result = rtype::render::vulkan::texture::premultiplySrgb(opaque);
    EXPECT_EQ(result, (std::vector<std::byte>{opaque.begin(), opaque.end()}));
}

TEST(PremultiplySrgb, ClearsFullyTransparentPixels) {
    const std::vector<std::byte> result = rtype::render::vulkan::texture::premultiplySrgb(pixel(255, 255, 255, 0));
    const auto cleared = pixel(0, 0, 0, 0);
    EXPECT_EQ(result, (std::vector<std::byte>{cleared.begin(), cleared.end()}));
}

TEST(PremultiplySrgb, MultipliesInLinearSpace) {
    // White at about half opacity: linear 1 * 128 / 255, which sRGB encodes as 188, not the 128 of multiplying the
    // bytes.
    const std::vector<std::byte> result = rtype::render::vulkan::texture::premultiplySrgb(pixel(255, 255, 255, 128));
    ASSERT_EQ(result.size(), 4U);
    for (std::size_t channel = 0; channel < 3; ++channel) {
        EXPECT_EQ(std::to_integer<int>(result.at(channel)), 188);
    }
    EXPECT_EQ(std::to_integer<int>(result.at(3)), 128);
}

TEST(PremultiplySrgb, KeepsBlackBlack) {
    const std::vector<std::byte> result = rtype::render::vulkan::texture::premultiplySrgb(pixel(0, 0, 0, 77));
    const auto expected = pixel(0, 0, 0, 77);
    EXPECT_EQ(result, (std::vector<std::byte>{expected.begin(), expected.end()}));
}

TEST(PremultiplySrgb, RejectsAPartialPixel) {
    const std::array<std::byte, 3> partial{};
    EXPECT_THROW(static_cast<void>(rtype::render::vulkan::texture::premultiplySrgb(partial)), std::invalid_argument);
}
