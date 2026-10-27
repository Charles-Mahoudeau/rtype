/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextPlugin
*/

#include "plugins/text/TextPlugin.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::text {

namespace {
/// @brief Render, client: hands the fonts' atlas images to TextureAssets.
void queueFontTextures(ecs::ResMut<FontAssets> fonts, ecs::ResMut<backend::TextureAssets> textures) {
    for (const FontAssets::TextureRequest& request : fonts->takeTextureRequests()) {
        try {
            static_cast<void>(textures->load(request.path, request.desc));
        } catch (const std::exception& error) {
            std::cerr << "[FontAssets] " << error.what() << '\n';
        }
    }
}

/// @brief Render, client: lays out the texts and pushes them to the SpriteQueue.
void drawTexts(ecs::Res<FontAssets> fonts, ecs::Res<backend::TextureAssets> textures,
               ecs::ResMut<backend::graphics::SpriteQueue> queue,
               ecs::Query<const transform::GlobalTransform, const TextRenderer> texts) {
    for (auto [entity, global, drawn] : texts) {
        const Font* font = fonts->get(drawn.font);
        if (font == nullptr || drawn.text.empty()) {
            continue;
        }
        const std::optional<backend::graphics::TextureId> texture = textures->getTexture(font->getTexture());
        if (!texture) {
            continue;
        }
        TextLayout layout = layoutText(*font, drawn.text, drawn.size, drawn.align);
        if (layout.glyphs.empty()) {
            continue;
        }
        queue->push(drawn.layer, backend::graphics::Text{.texture = *texture,
                                                         .glyphs = std::move(layout.glyphs),
                                                         .position = global.position,
                                                         .scale = global.scale,
                                                         .origin = layout.size * drawn.anchor,
                                                         .rotation = global.rotation,
                                                         .color = drawn.color,
                                                         .distanceRange = font->getDistanceRange(),
                                                         .blend = drawn.blend});
    }
}
}  // namespace

std::string_view TextPlugin::getName() const noexcept { return kName; }

void TextPlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<FontAssets>()) {
        resources.insert(FontAssets{});
    }
    if (!resources.has<backend::TextureAssets>()) {
        resources.insert(backend::TextureAssets{});
    }
    if (!resources.has<backend::graphics::SpriteQueue>()) {
        resources.insert(backend::graphics::SpriteQueue{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<TextRenderer>());
    app.addSystem(ecs::Phase::kRender, std::string{kTexturesSystem}, &queueFontTextures)
        .after(std::string{tilemap::TilemapPlugin::kTexturesSystem})
        .before(std::string{backend::BackendPlugin::kTexturesSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawTexts)
        .after(std::string{backend::BackendPlugin::kCameraSystem})
        .after(std::string{tilemap::TilemapPlugin::kDrawSystem})
        .before(std::string{backend::BackendPlugin::kSpritesSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::text
