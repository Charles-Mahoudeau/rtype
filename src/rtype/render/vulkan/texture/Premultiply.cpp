/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Premultiply
*/

#include "Premultiply.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <vector>

namespace rtype::render::vulkan::texture {

namespace {

/// @brief Values of an 8-bit channel.
constexpr std::size_t kChannelValues = 256;

/// @brief Bytes of an RGBA8 pixel.
constexpr std::size_t kPixelSize = 4;

/// @brief Index of the alpha byte in a pixel.
constexpr std::size_t kAlpha = 3;

/// @return The linear value of the sRGB-encoded @p encoded, 0 to 1.
double toLinear(double encoded) {
    return encoded <= 0.04045 ? encoded / 12.92 : std::pow((encoded + 0.055) / 1.055, 2.4);
}

/// @return The sRGB encoding of the linear @p linear, 0 to 1.
double toSrgb(double linear) {
    return linear <= 0.0031308 ? linear * 12.92 : (1.055 * std::pow(linear, 1.0 / 2.4)) - 0.055;
}

/// @brief The premultiplied sRGB byte of every (alpha, channel) pair, at index alpha * 256 + channel.
using PremultiplyTable = std::array<std::uint8_t, kChannelValues * kChannelValues>;

/// @return The table, computed on first use: 65536 conversions instead of three per pixel.
const PremultiplyTable& getTable() {
    static const PremultiplyTable table = [] {
        PremultiplyTable built{};
        const auto maxValue = static_cast<double>(kChannelValues - 1);
        for (std::size_t alpha = 0; alpha < kChannelValues; ++alpha) {
            for (std::size_t channel = 0; channel < kChannelValues; ++channel) {
                const double linear = toLinear(static_cast<double>(channel) / maxValue);
                const double premultiplied = toSrgb(linear * (static_cast<double>(alpha) / maxValue));
                built.at((alpha * kChannelValues) + channel) =
                    static_cast<std::uint8_t>(std::lround(premultiplied * maxValue));
            }
        }
        return built;
    }();
    return table;
}

}  // namespace

std::vector<std::byte> premultiplySrgb(std::span<const std::byte> rgba) {
    if (rgba.size() % kPixelSize != 0) {
        throw std::invalid_argument(
            std::format("premultiplySrgb(): {} bytes is not a whole number of RGBA8 pixels", rgba.size()));
    }
    const PremultiplyTable& table = getTable();
    std::vector<std::byte> premultiplied{rgba.begin(), rgba.end()};
    for (std::size_t pixel = 0; pixel < premultiplied.size(); pixel += kPixelSize) {
        const auto alpha = std::to_integer<std::size_t>(premultiplied.at(pixel + kAlpha));
        if (alpha == kChannelValues - 1) {
            continue;
        }
        for (std::size_t channel = 0; channel < kAlpha; ++channel) {
            std::byte& value = premultiplied.at(pixel + channel);
            value = std::byte{table.at((alpha * kChannelValues) + std::to_integer<std::size_t>(value))};
        }
    }
    return premultiplied;
}

}  // namespace rtype::render::vulkan::texture
