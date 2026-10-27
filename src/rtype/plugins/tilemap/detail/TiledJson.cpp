/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TiledJson
*/

#include "plugins/tilemap/detail/TiledJson.hpp"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/exceptions/TilemapExceptions.hpp"

namespace rtype::plugins::tilemap::detail {

namespace {
using Json = nlohmann::json;

/// @brief Highest value of one colour channel in Tiled's "#AARRGGBB" notation.
constexpr float kChannelMax = 255.0F;

/// @brief What a group passes on to the layers it holds.
struct Inherited {
    std::string prefix;                   ///< "group/" for each enclosing group.
    glm::vec2 offset{0.0F};               ///< Added.
    glm::vec2 parallax{1.0F};             ///< Multiplied.
    glm::vec2 scroll{0.0F};               ///< Added.
    float opacity = 1.0F;                 ///< Multiplied.
    backend::graphics::Color tint{1.0F};  ///< Multiplied.
    bool visible = true;                  ///< All must be visible.
};

/// @return @p path, normalized as AssetId::fromPath() sees it, with forward slashes.
std::string normalize(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }

/// @return The colour of Tiled's "#RRGGBB" or "#AARRGGBB" @p text.
backend::graphics::Color readColor(const std::string& text) {
    const std::string_view digits = text.starts_with('#') ? std::string_view{text}.substr(1) : std::string_view{text};
    if (digits.size() != 6 && digits.size() != 8) {
        throw exceptions::TilemapException{std::format("'{}' is not a colour", text)};
    }
    std::uint32_t value = 0;
    const auto [end, error] =
        std::from_chars(std::to_address(digits.begin()), std::to_address(digits.end()), value, 16);
    if (error != std::errc{} || end != std::to_address(digits.end())) {
        throw exceptions::TilemapException{std::format("'{}' is not a colour", text)};
    }
    const auto channel = [value](std::uint32_t shift) {
        return static_cast<float>((value >> shift) & 0xFFU) / kChannelMax;
    };
    const float alpha = digits.size() == 8 ? channel(24) : 1.0F;
    return {channel(16), channel(8), channel(0), alpha};
}

/// @return The custom float property @p name of @p object, or @p fallback.
float readProperty(const Json& object, std::string_view name, float fallback) {
    if (!object.contains("properties")) {
        return fallback;
    }
    for (const Json& property : object.at("properties")) {
        if (property.at("name").get<std::string>() == name) {
            return property.at("value").get<float>();
        }
    }
    return fallback;
}

/// @return What @p layer passes on, combined with what @p parent passed to it.
Inherited inherit(const Json& layer, const Inherited& parent) {
    Inherited combined = parent;
    combined.offset += glm::vec2{layer.value("offsetx", 0.0F), layer.value("offsety", 0.0F)};
    combined.parallax *= glm::vec2{layer.value("parallaxx", 1.0F), layer.value("parallaxy", 1.0F)};
    combined.scroll += glm::vec2{readProperty(layer, "scrollx", 0.0F), readProperty(layer, "scrolly", 0.0F)};
    combined.opacity *= layer.value("opacity", 1.0F);
    if (layer.contains("tintcolor")) {
        combined.tint *= readColor(layer.at("tintcolor").get<std::string>());
    }
    combined.visible = combined.visible && layer.value("visible", true);
    return combined;
}

/// @return The ids of a tile layer's (or chunk's) `data`, which must be a JSON array.
std::vector<std::uint32_t> readData(const Json& owner, const std::string& layerName) {
    if (owner.value("encoding", std::string{"csv"}) != "csv" || !owner.at("data").is_array()) {
        throw exceptions::TilemapException{
            std::format("layer '{}': only the CSV tile layer format is supported, not base64", layerName)};
    }
    return owner.at("data").get<std::vector<std::uint32_t>>();
}

/// @return The tileset described by @p object, its image relative to @p directory.
Tileset readTileset(const Json& object, const std::filesystem::path& directory, std::uint32_t firstGid) {
    const auto name = object.value("name", std::string{});
    if (!object.contains("image")) {
        throw exceptions::TilemapException{
            std::format("tileset '{}': a tileset of separate images is not supported, use one image", name)};
    }
    std::string imagePath = normalize(directory / object.at("image").get<std::string>());
    Tileset tileset{
        .name = name,
        .imagePath = imagePath,
        .texture = asset::AssetId::fromPath(imagePath),
        .imageSize = {object.at("imagewidth").get<std::uint32_t>(), object.at("imageheight").get<std::uint32_t>()},
        .tileSize = {object.at("tilewidth").get<std::uint32_t>(), object.at("tileheight").get<std::uint32_t>()},
        .firstGid = firstGid,
        .tileCount = object.at("tilecount").get<std::uint32_t>(),
        .columns = object.at("columns").get<std::uint32_t>(),
        .spacing = object.value("spacing", 0U),
        .margin = object.value("margin", 0U)};
    if (object.contains("tileoffset")) {
        const Json& offset = object.at("tileoffset");
        tileset.tileOffset = {offset.at("x").get<float>(), offset.at("y").get<float>()};
    }
    return tileset;
}

/// @brief Parses a map's layers, in order, into flat MapLayers.
class LayerReader {
  public:
    explicit LayerReader(std::filesystem::path directory) : _directory{std::move(directory)} {}
    ~LayerReader() = default;
    LayerReader(const LayerReader& other) = delete;
    LayerReader& operator=(const LayerReader& other) = delete;
    LayerReader(LayerReader&& other) = delete;
    LayerReader& operator=(LayerReader&& other) = delete;

    /// @return The layers read so far, bottom first.
    [[nodiscard]] std::vector<MapLayer>& getLayers() noexcept { return _layers; }

    /// @brief Reads @p layers, the map's top-level layers, groups included.
    void read(const Json& layers) {
        struct Pending {
            Json::const_iterator next;
            Json::const_iterator end;
            Inherited inherited;
        };
        std::vector<Pending> pending{{.next = layers.begin(), .end = layers.end(), .inherited = Inherited{}}};
        while (!pending.empty()) {
            Pending& group = pending.back();
            if (group.next == group.end) {
                pending.pop_back();
                continue;
            }
            const Json& layer = *group.next++;
            const auto type = layer.at("type").get<std::string>();
            const Inherited own = inherit(layer, group.inherited);
            const std::string name = group.inherited.prefix + layer.value("name", std::string{});
            if (type == "group") {
                Inherited children = own;
                children.prefix = name + "/";
                const Json& sublayers = layer.at("layers");
                pending.push_back(
                    {.next = sublayers.begin(), .end = sublayers.end(), .inherited = std::move(children)});
            } else if (type == "tilelayer") {
                readTiles(layer, own, name);
            } else if (type == "imagelayer") {
                readImage(layer, own, name);
            }
        }
    }

  private:
    /// @return A layer named @p name with what @p own gives it.
    static MapLayer makeLayer(const Inherited& own, std::string name, LayerKind kind) {
        return MapLayer{.name = std::move(name),
                        .kind = kind,
                        .offset = own.offset,
                        .parallax = own.parallax,
                        .scroll = own.scroll,
                        .opacity = own.opacity,
                        .tint = own.tint,
                        .visible = own.visible};
    }

    /// @brief Reads the tile layer @p layer.
    void readTiles(const Json& layer, const Inherited& own, const std::string& name) {
        MapLayer tiles = makeLayer(own, name, LayerKind::kTiles);
        if (layer.contains("chunks")) {
            for (const Json& chunk : layer.at("chunks")) {
                setTiles(tiles, {chunk.at("x").get<std::int32_t>(), chunk.at("y").get<std::int32_t>()},
                         chunk.at("width").get<std::uint32_t>(), readData(chunk, name));
            }
        } else {
            setTiles(tiles, {layer.value("x", 0), layer.value("y", 0)}, layer.at("width").get<std::uint32_t>(),
                     readData(layer, name));
        }
        _layers.push_back(std::move(tiles));
    }

    /// @brief Reads the image layer @p layer; one without an image is skipped.
    void readImage(const Json& layer, const Inherited& own, std::string name) {
        const auto image = layer.value("image", std::string{});
        if (image.empty()) {
            return;
        }
        MapLayer picture = makeLayer(own, std::move(name), LayerKind::kImage);
        picture.imagePath = normalize(_directory / image);
        picture.image = asset::AssetId::fromPath(picture.imagePath);
        picture.repeatX = layer.value("repeatx", false);
        picture.repeatY = layer.value("repeaty", false);
        _layers.push_back(std::move(picture));
    }

    std::filesystem::path _directory;  ///< Where the map is: image paths are relative to it.
    std::vector<MapLayer> _layers;     ///< Bottom first.
};
}  // namespace

Tilemap parseTiledMap(std::string_view text, const std::filesystem::path& path, const TextReader& readText) {
    const std::string name = path.generic_string();
    try {
        const Json root = Json::parse(text);
        const auto orientation = root.value("orientation", std::string{"orthogonal"});
        if (orientation != "orthogonal") {
            throw exceptions::TilemapException{
                std::format("only orthogonal maps are supported, this one is {}", orientation)};
        }
        const std::filesystem::path directory = path.parent_path();
        std::vector<Tileset> tilesets;
        for (const Json& entry : root.value("tilesets", Json::array())) {
            const auto firstGid = entry.at("firstgid").get<std::uint32_t>();
            if (entry.contains("source")) {
                const std::filesystem::path source = directory / entry.at("source").get<std::string>();
                tilesets.push_back(readTileset(Json::parse(readText(source)), source.parent_path(), firstGid));
            } else {
                tilesets.push_back(readTileset(entry, directory, firstGid));
            }
        }
        LayerReader layers{directory};
        layers.read(root.at("layers"));
        return Tilemap{{root.at("tilewidth").get<std::uint32_t>(), root.at("tileheight").get<std::uint32_t>()},
                       std::move(tilesets),
                       std::move(layers.getLayers()),
                       {root.value("parallaxoriginx", 0.0F), root.value("parallaxoriginy", 0.0F)}};
    } catch (const nlohmann::json::exception& error) {
        throw exceptions::TilemapException{std::format("Tilemap '{}': {}", name, error.what())};
    } catch (const exceptions::TilemapException& error) {
        throw exceptions::TilemapException{std::format("Tilemap '{}': {}", name, error.what())};
    }
}

}  // namespace rtype::plugins::tilemap::detail
