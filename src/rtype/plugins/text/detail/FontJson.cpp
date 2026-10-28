/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FontJson
*/

#include "plugins/text/detail/FontJson.hpp"

#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

#include "plugins/asset/AssetFiles.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/exceptions/TextExceptions.hpp"

namespace rtype::plugins::text::detail {

namespace {
using Json = nlohmann::json;

/// @brief The edges of a box as msdf-atlas-gen writes them.
struct Edges {
    float left = 0.0F;    ///< Smallest X.
    float top = 0.0F;     ///< Y of the edge msdf-atlas-gen calls the top.
    float right = 0.0F;   ///< Largest X.
    float bottom = 0.0F;  ///< Y of the edge msdf-atlas-gen calls the bottom.
};

/// @return The edges of @p object: {left, top, right, bottom}.
Edges readEdges(const Json& object) {
    return Edges{.left = object.at("left").get<float>(),
                 .top = object.at("top").get<float>(),
                 .right = object.at("right").get<float>(),
                 .bottom = object.at("bottom").get<float>()};
}

/// @return @p edges as a rectangle Y down: its top is the smaller Y of the two edges.
backend::math::Rect toRect(const Edges& edges) {
    const float top = edges.top < edges.bottom ? edges.top : edges.bottom;
    const float bottom = edges.top < edges.bottom ? edges.bottom : edges.top;
    return backend::math::Rect{glm::vec2{edges.left, top}, glm::vec2{edges.right - edges.left, bottom - top}};
}

/// @brief Reads the glyphs and the kerning of @p root into @p font. A file written Y up (@p yUp, yOrigin "bottom") is
/// flipped Y down, its atlas bounds from the bottom of the @p atlasHeight pixels high image.
void readGlyphs(const Json& root, Font& font, bool yUp, float atlasHeight) {
    for (const Json& entry : root.at("glyphs")) {
        Glyph glyph{.advance = entry.at("advance").get<float>()};
        if (entry.contains("planeBounds") && entry.contains("atlasBounds")) {
            Edges plane = readEdges(entry.at("planeBounds"));
            Edges atlas = readEdges(entry.at("atlasBounds"));
            if (yUp) {
                plane.top = -plane.top;
                plane.bottom = -plane.bottom;
                atlas.top = atlasHeight - atlas.top;
                atlas.bottom = atlasHeight - atlas.bottom;
            }
            glyph.plane = toRect(plane);
            glyph.atlas = toRect(atlas);
        }
        font.addGlyph(entry.at("unicode").get<char32_t>(), glyph);
    }
    for (const Json& pair : root.value("kerning", Json::array())) {
        font.setKerning(pair.at("unicode1").get<char32_t>(), pair.at("unicode2").get<char32_t>(),
                        pair.at("advance").get<float>());
    }
}
}  // namespace

Font parseFontJson(std::string_view text, const std::filesystem::path& path) {
    const std::string name = path.generic_string();
    try {
        const Json root = Json::parse(text);
        const Json& atlas = root.at("atlas");
        const auto type = atlas.at("type").get<std::string>();
        if (type != "msdf" && type != "mtsdf") {
            throw exceptions::FontException{
                std::format("a '{}' atlas, only msdf and mtsdf ones are supported (msdf-atlas-gen -type msdf)", type)};
        }
        const bool yUp = atlas.value("yOrigin", std::string{"bottom"}) == "bottom";
        const glm::uvec2 size{atlas.at("width").get<std::uint32_t>(), atlas.at("height").get<std::uint32_t>()};
        const Json& metrics = root.at("metrics");
        const float flip = yUp ? -1.0F : 1.0F;
        std::filesystem::path image = path;
        image.replace_extension(".png");
        Font font{asset::normalizePath(image),
                  backend::graphics::TextureDesc{.size = size,
                                                 .filter = backend::graphics::TextureFilter::kLinear,
                                                 .encoding = backend::graphics::TextureEncoding::kLinear},
                  atlas.at("distanceRange").get<float>(),
                  Font::Metrics{.lineHeight = metrics.at("lineHeight").get<float>(),
                                .ascender = metrics.at("ascender").get<float>() * flip,
                                .descender = metrics.at("descender").get<float>() * flip}};
        readGlyphs(root, font, yUp, static_cast<float>(size.y));
        return font;
    } catch (const nlohmann::json::exception& error) {
        throw exceptions::FontException{std::format("Font '{}': {}", name, error.what())};
    } catch (const exceptions::FontException& error) {
        throw exceptions::FontException{std::format("Font '{}': {}", name, error.what())};
    }
}

}  // namespace rtype::plugins::text::detail
