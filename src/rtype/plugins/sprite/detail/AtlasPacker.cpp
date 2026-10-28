/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasPacker
*/

#include "plugins/sprite/detail/AtlasPacker.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "plugins/backend/graphics/ImageDecoding.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

namespace rtype::plugins::sprite::detail {

namespace {
/// @brief Bytes of one pixel: RGBA, 8 bits per channel.
constexpr std::size_t kChannels = 4;

/// @brief The side the search for the packed image's size starts from, in pixels.
constexpr std::uint32_t kMinPackedSize = 64;

/// @brief One decoded image.
struct Decoded {
    std::string name;             ///< The file's stem.
    backend::DecodedImage image;  ///< Its pixels.
};

/// @return The RGBA pixels of the image file at @p path.
/// @throws core::asset::exceptions::AssetLoadException If it cannot be read or decoded.
Decoded decode(const std::filesystem::path& path) {
    return Decoded{.name = path.stem().string(),
                   .image = backend::decodeImage(path.generic_string(), backend::readFile(path))};
}

/// @return Where each image goes in a @p size image, @p padding pixels apart, or nothing if they do not all fit.
std::optional<std::vector<stbrp_rect>> tryPack(const std::vector<Decoded>& images, glm::uvec2 size,
                                               std::uint32_t padding) {
    std::vector<stbrp_rect> rects(images.size());
    for (std::size_t index = 0; index < images.size(); ++index) {
        stbrp_rect& rect = rects.at(index);
        rect.id = static_cast<int>(index);
        rect.w = static_cast<int>(images.at(index).image.size.x + padding);
        rect.h = static_cast<int>(images.at(index).image.size.y + padding);
    }
    stbrp_context context{};
    std::vector<stbrp_node> nodes(size.x);
    stbrp_init_target(&context, static_cast<int>(size.x), static_cast<int>(size.y), nodes.data(),
                      static_cast<int>(nodes.size()));
    if (stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size())) == 0) {
        return std::nullopt;
    }
    return rects;
}
}  // namespace

PackedImages packImages(std::span<const std::filesystem::path> images, std::uint32_t padding) {
    if (images.empty()) {
        throw exceptions::AtlasException{"no image to pack"};
    }
    std::vector<Decoded> decoded;
    std::unordered_set<std::string> names;
    for (const std::filesystem::path& path : images) {
        decoded.push_back(decode(path));
        if (!names.insert(decoded.back().name).second) {
            throw exceptions::AtlasException{
                std::format("two images are named '{}': frames are named after the file", decoded.back().name)};
        }
    }
    for (std::uint32_t side = kMinPackedSize; side <= kMaxPackedSize; side *= 2) {
        for (const glm::uvec2 size : {glm::uvec2{side, side}, glm::uvec2{std::min(side * 2, kMaxPackedSize), side}}) {
            const std::optional<std::vector<stbrp_rect>> rects = tryPack(decoded, size, padding);
            if (!rects) {
                continue;
            }
            PackedImages packed{
                .size = size, .pixels = std::vector<std::byte>(static_cast<std::size_t>(size.x) * size.y * kChannels)};
            // stbrp_pack_rects() gives the rectangles back in the order they were given: the images' order.
            for (const stbrp_rect& rect : *rects) {
                const Decoded& decodedImage = decoded.at(static_cast<std::size_t>(rect.id));
                const backend::DecodedImage& image = decodedImage.image;
                const glm::uvec2 corner{static_cast<std::uint32_t>(rect.x), static_cast<std::uint32_t>(rect.y)};
                const std::size_t rowBytes = image.size.x * kChannels;
                const std::span<const std::byte> source{image.pixels};
                for (std::uint32_t row = 0; row < image.size.y; ++row) {
                    const std::span<const std::byte> line = source.subspan(row * rowBytes, rowBytes);
                    const std::size_t offset =
                        ((static_cast<std::size_t>(corner.y + row) * size.x) + corner.x) * kChannels;
                    std::ranges::copy(line, std::next(packed.pixels.begin(), static_cast<std::ptrdiff_t>(offset)));
                }
                packed.placements.push_back(
                    PackedImages::Placement{.name = decodedImage.name,
                                            .source = backend::math::Rect{glm::vec2{corner}, glm::vec2{image.size}}});
            }
            return packed;
        }
    }
    throw exceptions::AtlasException{
        std::format("the images do not fit in {}x{} pixels", kMaxPackedSize, kMaxPackedSize)};
}

}  // namespace rtype::plugins::sprite::detail
