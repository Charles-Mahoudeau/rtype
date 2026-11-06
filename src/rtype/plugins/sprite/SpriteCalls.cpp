/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteCalls
*/

#include "plugins/sprite/SpriteCalls.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::sprite {
namespace {
/// @return How a texture is sampled: sharp pixels, or smooth when @p linear.
[[nodiscard]] contract::renderer::TextureDesc textureDesc(const bool linear) noexcept {
    contract::renderer::TextureDesc desc;
    desc.filter = linear ? contract::renderer::TextureFilter::kLinear : contract::renderer::TextureFilter::kNearest;
    return desc;
}

/// @return The atlas @p id names, to change it.
/// @throws exceptions::AtlasException If no atlas has this id.
[[nodiscard]] SpriteAtlas& requireAtlas(AtlasAssets& atlases, const core::asset::AssetId id) {
    SpriteAtlas* atlas = atlases.edit(id);
    if (atlas == nullptr) {
        throw exceptions::AtlasException{std::format("No atlas has the id {}", id.value)};
    }
    return *atlas;
}

/// @return The direction named @p name.
/// @throws exceptions::AtlasException If no direction has this name.
[[nodiscard]] AnimationDirection requireDirection(const std::string_view name) {
    if (name == "forward") {
        return AnimationDirection::kForward;
    }
    if (name == "reverse") {
        return AnimationDirection::kReverse;
    }
    if (name == "pingPong") {
        return AnimationDirection::kPingPong;
    }
    throw exceptions::AtlasException{
        std::format("Unknown animation direction '{}' (expected 'forward', 'reverse' or 'pingPong')", name)};
}

AtlasResult loadAtlas(ecs::ResMut<AtlasAssets> atlases, const LoadAtlasArguments& arguments) {
    const contract::renderer::TextureDesc desc = textureDesc(arguments.linear);
    return AtlasResult{.atlas = atlases->load(std::filesystem::path{arguments.path.view()}, desc)};
}

AtlasResult createAtlas(ecs::ResMut<AtlasAssets> atlases, const CreateAtlasArguments& arguments) {
    if (arguments.texture.empty()) {
        throw exceptions::AtlasException{"An atlas needs a texture"};
    }
    std::string texture{arguments.texture.view()};
    const std::string name{arguments.name.empty() ? arguments.texture.view() : arguments.name.view()};
    const contract::renderer::TextureDesc desc = textureDesc(arguments.linear);
    SpriteAtlas atlas = arguments.count == 0
                            ? SpriteAtlas{std::move(texture), desc}
                            : SpriteAtlas::grid(std::move(texture), arguments.frameSize, arguments.count,
                                                arguments.spacing, arguments.columns, arguments.offset, desc);
    return AtlasResult{.atlas = atlases->add(name, std::move(atlas))};
}

FrameResult addFrame(ecs::ResMut<AtlasAssets> atlases, const AddFrameArguments& arguments) {
    SpriteAtlas& atlas = requireAtlas(*atlases, arguments.atlas);
    const auto index = static_cast<std::uint32_t>(atlas.getFrames().size());
    std::string name = arguments.name.empty() ? std::format("{}", index) : std::string{arguments.name.view()};
    std::optional<glm::vec2> origin;
    if (!std::isnan(arguments.origin.x) && !std::isnan(arguments.origin.y)) {
        origin = arguments.origin;
    }
    atlas.addFrame(std::move(name), contract::renderer::math::Rect{arguments.position, arguments.size}, origin);
    return FrameResult{.frame = index};
}

void addAnimation(ecs::ResMut<AtlasAssets> atlases, const AddAnimationArguments& arguments) {
    SpriteAtlas& atlas = requireAtlas(*atlases, arguments.atlas);
    const auto frameCount = static_cast<std::uint32_t>(atlas.getFrames().size());
    std::uint32_t count = arguments.count;
    if (count == 0) {
        // Up to the last frame; one frame past the end when `first` is not a frame, which addAnimation() refuses.
        count = arguments.first < frameCount ? frameCount - arguments.first : 1;
    }
    std::vector<std::uint32_t> frames;
    frames.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        frames.push_back(arguments.first + index);
    }
    atlas.addAnimation(std::string{arguments.name.view()}, std::move(frames), arguments.frameDuration,
                       requireDirection(arguments.direction), arguments.plays);
}
}  // namespace

void addSpriteCalls(ecs::App& app) {
    app.addCall(std::string{kLoadAtlasCall}, &loadAtlas);
    app.addCall(std::string{kCreateAtlasCall}, &createAtlas);
    app.addCall(std::string{kAddFrameCall}, &addFrame);
    app.addCall(std::string{kAddAnimationCall}, &addAnimation);
}

}  // namespace rtype::plugins::sprite
