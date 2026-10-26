/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePlugin
*/

#include "plugins/sprite/SpritePlugin.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/sprite/AnimationEvents.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAnimator.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/Time.hpp"

namespace rtype::plugins::sprite {

namespace {
/// @return Whether @p left and @p right are the same rectangle.
bool sameRect(const backend::math::Rect& left, const backend::math::Rect& right) noexcept {
    return left.getX() == right.getX() && left.getY() == right.getY() && left.getWidth() == right.getWidth() &&
           left.getHeight() == right.getHeight();
}

/// @brief Shows @p frame of @p atlas in @p renderer, writing only what changes so an unchanged sprite stays unchanged
/// for change detection and replication.
void showFrame(ecs::Mut<backend::SpriteRenderer>& renderer, const SpriteAtlas& atlas, const AtlasFrame& frame) {
    const backend::SpriteRenderer& current = renderer.get();
    if (current.texture == atlas.getTexture() && sameRect(current.source, frame.source) &&
        current.origin == frame.origin) {
        return;
    }
    renderer->texture = atlas.getTexture();
    renderer->source = frame.source;
    renderer->origin = frame.origin;
}

/// @brief PostUpdate: advances the animations and shows their frame.
void animateSprites(ecs::Res<AtlasAssets> atlases, ecs::Res<ecs::Time> time,
                    ecs::Query<SpriteAnimator, ecs::Optional<backend::SpriteRenderer>> animators,
                    ecs::EventWriter<AnimationFinished> finished) {
    for (auto [entity, animator, renderer] : animators) {
        const SpriteAtlas* atlas = atlases->get(animator.get().atlas);
        const SpriteAnimation* animation = atlas != nullptr ? atlas->findAnimation(animator.get().animation) : nullptr;
        if (animation == nullptr) {
            continue;
        }
        if (animator.get().playing) {
            animator->time += time->delta * animator.get().speed;
        }
        const SpriteAnimation::Sample sample = animation->sample(animator.get().time);
        if (sample.finished && animator.get().playing) {
            animator->playing = false;
            finished.send(AnimationFinished{.entity = entity, .animation = animator.get().animation});
        }
        if (renderer) {
            showFrame(renderer, *atlas, atlas->getFrames().at(sample.frame));
        }
    }
}

/// @brief PostUpdate, client: hands the atlases' textures to TextureAssets.
void queueAtlasTextures(ecs::ResMut<AtlasAssets> atlases, ecs::ResMut<backend::TextureAssets> textures) {
    for (AtlasAssets::TextureRequest& request : atlases->takeTextureRequests()) {
        try {
            if (request.pixels.empty()) {
                static_cast<void>(textures->load(request.path, request.desc));
            } else {
                static_cast<void>(textures->loadPixels(request.path, request.pixels, request.desc));
            }
        } catch (const std::exception& error) {
            std::cerr << "[AtlasAssets] " << error.what() << '\n';
        }
    }
}
}  // namespace

std::string_view SpritePlugin::getName() const noexcept { return kName; }

void SpritePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<AtlasAssets>()) {
        resources.insert(AtlasAssets{});
    }
    if (!resources.has<backend::TextureAssets>()) {
        resources.insert(backend::TextureAssets{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<SpriteAnimator>());
    static_cast<void>(app.getWorld().getEvents().registerType<AnimationFinished>());
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kAnimateSystem}, &animateSprites).setSide(ecs::Side::kBoth);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kTexturesSystem}, &queueAtlasTextures)
        .after(std::string{kAnimateSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::sprite
