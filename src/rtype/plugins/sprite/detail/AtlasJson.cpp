/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasJson
*/

#include "plugins/sprite/detail/AtlasJson.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "plugins/asset/AssetFiles.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

namespace rtype::plugins::sprite::detail {

namespace {
/// @brief The parsed file. Ordered: an object's frames keep the file's order, which is the animations' order (a sorted
/// map would put "ship 10" before "ship 2").
using Json = nlohmann::ordered_json;

/// @brief Milliseconds a frame lasts when the file does not say.
constexpr float kDefaultDurationMs = 100.0F;

/// @brief Milliseconds per second.
constexpr float kMsPerSecond = 1000.0F;

/// @return @p object's {x, y} as a vector.
glm::vec2 readPoint(const Json& object) { return {object.at("x").get<float>(), object.at("y").get<float>()}; }

/// @return @p object's {w, h} as a vector.
glm::vec2 readSize(const Json& object) { return {object.at("w").get<float>(), object.at("h").get<float>()}; }

/// @brief A frame as the file describes it, before it joins the atlas.
struct ParsedFrame {
    std::string name;            ///< Its key (JSON Hash) or its "filename" (JSON Array).
    backend::math::Rect source;  ///< Where it is in the image.
    glm::vec2 origin{0.0F};      ///< Its pivot, in pixels from the top-left of source.
    float duration = 0.0F;       ///< Seconds.
};

/// @return The frame described by @p frame, named @p name.
ParsedFrame readFrame(std::string name, const Json& frame) {
    const Json& rect = frame.at("frame");
    const glm::vec2 size = readSize(rect);
    // A trimmed frame lost its transparent border: its pivot is placed in the untrimmed frame, then moved by the trim.
    const glm::vec2 untrimmed = frame.contains("sourceSize") ? readSize(frame.at("sourceSize")) : size;
    const glm::vec2 trim = frame.contains("spriteSourceSize") ? readPoint(frame.at("spriteSourceSize")) : glm::vec2{};
    const glm::vec2 pivot = frame.contains("pivot") ? readPoint(frame.at("pivot")) : glm::vec2{0.5F};
    return ParsedFrame{.name = std::move(name),
                       .source = backend::math::Rect{readPoint(rect), size},
                       .origin = (pivot * untrimmed) - trim,
                       .duration = frame.value("duration", kDefaultDurationMs) / kMsPerSecond};
}

/// @return The frames of @p frames, an object (JSON Hash) or an array (JSON Array), in the file's order.
std::vector<ParsedFrame> readFrames(const Json& frames) {
    std::vector<ParsedFrame> parsed;
    if (frames.is_object()) {
        for (const auto& [name, frame] : frames.items()) {
            parsed.push_back(readFrame(name, frame));
        }
    } else {
        for (const Json& frame : frames) {
            parsed.push_back(readFrame(frame.at("filename").get<std::string>(), frame));
        }
    }
    return parsed;
}

/// @return How many times @p tag repeats: Aseprite writes it as a string ("2"), other tools as a number; 0 (forever)
/// when absent.
std::uint32_t readRepeat(const Json& tag) {
    if (!tag.contains("repeat")) {
        return 0;
    }
    const Json& repeat = tag.at("repeat");
    if (repeat.is_number_unsigned()) {
        return repeat.get<std::uint32_t>();
    }
    const std::string text = repeat.get<std::string>();
    std::uint32_t count = 0;
    const auto [end, error] = std::from_chars(text.c_str(), std::to_address(text.end()), count);
    if (error != std::errc{} || end != std::to_address(text.end())) {
        throw exceptions::AtlasException{
            std::format("tag '{}': repeat '{}' is not a count", tag.at("name").get<std::string>(), text)};
    }
    return count;
}

/// @return The animation of the tag @p tag, over @p frames.
SpriteAnimation readTag(const Json& tag, const std::vector<ParsedFrame>& frames) {
    const auto name = tag.at("name").get<std::string>();
    const auto from = tag.at("from").get<std::uint32_t>();
    const auto to = tag.at("to").get<std::uint32_t>();
    if (from > to || to >= frames.size()) {
        throw exceptions::AtlasException{
            std::format("tag '{}': frames {} to {}, but the atlas has {}", name, from, to, frames.size())};
    }
    const std::string direction = tag.value("direction", std::string{"forward"});
    SpriteAnimation animation{.name = name, .repeat = readRepeat(tag)};
    for (std::uint32_t frame = from; frame <= to; ++frame) {
        animation.frames.push_back(frame);
        animation.durations.push_back(frames.at(frame).duration);
    }
    if (direction == "reverse") {
        animation.direction = AnimationDirection::kReverse;
    } else if (direction == "pingpong" || direction == "pingpong_reverse") {
        animation.direction = AnimationDirection::kPingPong;
        if (direction == "pingpong_reverse") {
            std::ranges::reverse(animation.frames);
            std::ranges::reverse(animation.durations);
        }
    } else if (direction != "forward") {
        throw exceptions::AtlasException{std::format("tag '{}': unknown direction '{}'", name, direction)};
    }
    return animation;
}
}  // namespace

SpriteAtlas parseAtlasJson(std::string_view text, const std::filesystem::path& path,
                           const backend::graphics::TextureDesc& desc) {
    const std::string name = path.generic_string();
    try {
        const Json root = Json::parse(text);
        const Json& meta = root.at("meta");
        const std::filesystem::path image = path.parent_path() / meta.at("image").get<std::string>();
        SpriteAtlas atlas{asset::normalizePath(image), desc};
        const std::vector<ParsedFrame> frames = readFrames(root.at("frames"));
        for (const ParsedFrame& frame : frames) {
            atlas.addFrame(frame.name, frame.source, frame.origin);
        }
        if (meta.contains("frameTags")) {
            for (const Json& tag : meta.at("frameTags")) {
                atlas.addAnimation(readTag(tag, frames));
            }
        }
        return atlas;
    } catch (const nlohmann::json::exception& error) {
        throw exceptions::AtlasException{std::format("Atlas '{}': {}", name, error.what())};
    } catch (const exceptions::AtlasException& error) {
        throw exceptions::AtlasException{std::format("Atlas '{}': {}", name, error.what())};
    }
}

}  // namespace rtype::plugins::sprite::detail
