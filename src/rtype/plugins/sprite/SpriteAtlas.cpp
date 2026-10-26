/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteAtlas
*/

#include "plugins/sprite/SpriteAtlas.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

namespace rtype::plugins::sprite {

namespace {
/// @return How many frames one pass of @p animation shows: every frame, or there and back without repeating the ends.
std::size_t stepsPerCycle(const SpriteAnimation& animation) noexcept {
    const std::size_t count = animation.frames.size();
    return animation.direction == AnimationDirection::kPingPong && count > 2 ? (2 * count) - 2 : count;
}

/// @return The position in animation.frames of the @p step-th frame of a pass.
std::size_t frameOfStep(const SpriteAnimation& animation, std::size_t step) noexcept {
    const std::size_t count = animation.frames.size();
    switch (animation.direction) {
        case AnimationDirection::kReverse:
            return count - 1 - step;
        case AnimationDirection::kPingPong:
            return step < count ? step : (2 * count) - 2 - step;
        case AnimationDirection::kForward:
            break;
    }
    return step;
}
}  // namespace

float SpriteAnimation::getCycleDuration() const noexcept {
    float total = 0.0F;
    const std::size_t steps = stepsPerCycle(*this);
    for (std::size_t step = 0; step < steps; ++step) {
        const std::size_t index = frameOfStep(*this, step);
        total += index < durations.size() ? durations.at(index) : 0.0F;
    }
    return total;
}

SpriteAnimation::Sample SpriteAnimation::sample(float time) const noexcept {
    if (frames.empty()) {
        return {};
    }
    const std::size_t steps = stepsPerCycle(*this);
    const float cycle = getCycleDuration();
    const std::uint32_t last = frames.at(frameOfStep(*this, steps - 1));
    if (cycle <= 0.0F) {
        return {.frame = frames.at(frameOfStep(*this, 0)), .finished = repeat != 0};
    }
    if (repeat != 0 && time >= cycle * static_cast<float>(repeat)) {
        return {.frame = last, .finished = true};
    }
    float inCycle = std::fmod(std::max(time, 0.0F), cycle);
    for (std::size_t step = 0; step < steps; ++step) {
        const std::size_t index = frameOfStep(*this, step);
        const float duration = index < durations.size() ? durations.at(index) : 0.0F;
        if (inCycle < duration) {
            return {.frame = frames.at(index)};
        }
        inCycle -= duration;
    }
    return {.frame = last};
}

SpriteAtlas::SpriteAtlas(std::string texturePath, const backend::graphics::TextureDesc& desc)
    : _texturePath{std::move(texturePath)}, _texture{asset::AssetId::fromPath(_texturePath)}, _textureDesc{desc} {}

SpriteAtlas SpriteAtlas::grid(std::string texturePath, glm::vec2 frameSize, std::uint32_t count, glm::vec2 spacing,
                              std::uint32_t columns, glm::vec2 offset, const backend::graphics::TextureDesc& desc) {
    SpriteAtlas atlas{std::move(texturePath), desc};
    const std::uint32_t perRow = columns == 0 ? count : columns;
    for (std::uint32_t index = 0; index < count; ++index) {
        const glm::vec2 cell{static_cast<float>(index % perRow), static_cast<float>(index / perRow)};
        atlas.addFrame(std::format("{}", index),
                       backend::math::Rect{offset + (cell * (frameSize + spacing)), frameSize});
    }
    return atlas;
}

SpriteAtlas& SpriteAtlas::addFrame(std::string name, const backend::math::Rect& source,
                                   std::optional<glm::vec2> origin) {
    if (_frameIndices.contains(name)) {
        throw exceptions::AtlasException{std::format("Atlas '{}': two frames are named '{}'", _texturePath, name)};
    }
    const glm::vec2 pivot = origin.value_or(glm::vec2{source.getWidth(), source.getHeight()} * 0.5F);
    _frameIndices.emplace(name, static_cast<std::uint32_t>(_frames.size()));
    _frames.push_back(AtlasFrame{.name = std::move(name), .source = source, .origin = pivot});
    return *this;
}

SpriteAtlas& SpriteAtlas::addAnimation(std::string name, std::vector<std::uint32_t> frames, float frameDuration,
                                       AnimationDirection direction, std::uint32_t repeat) {
    std::vector<float> durations(frames.size(), frameDuration);
    return addAnimation(SpriteAnimation{.name = std::move(name),
                                        .frames = std::move(frames),
                                        .durations = std::move(durations),
                                        .direction = direction,
                                        .repeat = repeat});
}

SpriteAtlas& SpriteAtlas::addAnimation(SpriteAnimation animation) {
    if (animation.durations.size() != animation.frames.size()) {
        throw exceptions::AtlasException{std::format("Atlas '{}': animation '{}' has {} frames but {} durations",
                                                     _texturePath, animation.name, animation.frames.size(),
                                                     animation.durations.size())};
    }
    for (const std::uint32_t frame : animation.frames) {
        if (frame >= _frames.size()) {
            throw exceptions::AtlasException{std::format("Atlas '{}': animation '{}' shows frame {}, but it has {}",
                                                         _texturePath, animation.name, frame, _frames.size())};
        }
    }
    const asset::AssetId id = animationId(animation.name);
    if (_animations.contains(id)) {
        throw exceptions::AtlasException{
            std::format("Atlas '{}': two animations are named '{}'", _texturePath, animation.name)};
    }
    _animations.emplace(id, std::move(animation));
    return *this;
}

std::optional<std::uint32_t> SpriteAtlas::findFrame(std::string_view name) const {
    const auto found = _frameIndices.find(std::string{name});
    return found != _frameIndices.end() ? std::optional{found->second} : std::nullopt;
}

const SpriteAnimation* SpriteAtlas::findAnimation(asset::AssetId id) const {
    const auto found = _animations.find(id);
    return found != _animations.end() ? &found->second : nullptr;
}

asset::AssetId animationId(std::string_view name) { return asset::AssetId::fromPath(std::filesystem::path{name}); }

}  // namespace rtype::plugins::sprite
