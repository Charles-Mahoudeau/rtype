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

#include "plugins/backend/Names.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
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

/// @brief Render, client: lays out the texts and pushes them to the SpriteQueue. A text whose material is created is
/// drawn with it, and with its MaterialInstance's values or else its shader's defaults.
void drawTexts(
    ecs::Res<FontAssets> fonts, ecs::Res<backend::TextureAssets> textures, ecs::Res<shader::ShaderHandles> handles,
    ecs::ResMut<backend::graphics::SpriteQueue> queue,
    ecs::Query<const transform::GlobalTransform, const TextRenderer, ecs::Optional<const shader::MaterialInstance>>
        texts) {
    for (auto [entity, global, drawn, instance] : texts) {
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
        backend::graphics::Text text{.texture = *texture,
                                     .glyphs = std::move(layout.glyphs),
                                     .position = global.position,
                                     .scale = global.scale,
                                     .origin = layout.size * drawn.anchor,
                                     .rotation = global.rotation,
                                     .color = drawn.color,
                                     .distanceRange = font->getDistanceRange(),
                                     .blend = drawn.blend};
        if (const shader::ShaderHandles::Material* material = handles->getMaterial(drawn.material)) {
            text.material = material->material;
            text.instanceValues = instance != nullptr ? instance->values : material->instanceDefaults;
        }
        queue->push(drawn.layer, std::move(text));
    }
}
}  // namespace

std::string_view TextPlugin::getName() const noexcept { return kName; }

void TextPlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<FontAssets>();
    resources.insertIfAbsent<backend::TextureAssets>();
    resources.insertIfAbsent<backend::graphics::SpriteQueue>();
    static_cast<void>(app.getWorld().getComponents().registerType<TextRenderer>());
    app.addSystem(ecs::Phase::kRender, std::string{kTexturesSystem}, &queueFontTextures)
        .after(std::string{backend::names::kTilemapTextures})
        .before(std::string{backend::names::kBackendTextures})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawTexts)
        .after(std::string{backend::names::kBackendCamera})
        .after(std::string{backend::names::kTilemapDraw})
        .before(std::string{backend::names::kBackendSprites})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::text
