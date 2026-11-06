/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextPlugin
*/

#include "plugins/text/TextPlugin.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "plugins/AssetDirectory.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/SpriteQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextCalls.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
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
void queueFontTextures(ecs::ResMut<FontAssets> fonts, ecs::ResAppend<graphics::TextureAssets> textures) {
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
    ecs::Res<FontAssets> fonts, ecs::Res<graphics::TextureAssets> textures, ecs::Res<shader::ShaderHandles> handles,
    ecs::ResAppend<graphics::SpriteQueue> queue,
    ecs::Query<const transform::GlobalTransform, const TextRenderer, ecs::Optional<const shader::MaterialInstance>>
        texts) {
    for (auto [entity, global, drawn, instance] : texts) {
        const Font* font = fonts->get(drawn.font);
        if (font == nullptr || drawn.text.empty()) {
            continue;
        }
        const std::optional<contract::renderer::TextureId> texture = textures->getTexture(font->getTexture());
        if (!texture) {
            continue;
        }
        TextLayout layout = layoutText(*font, drawn.text, drawn.size, drawn.align);
        if (layout.glyphs.empty()) {
            continue;
        }
        contract::renderer::Text text{.texture = *texture,
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

std::span<const std::string_view> TextPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 3> kDependencies{
        graphics::GraphicsPlugin::kName, shader::ShaderPlugin::kName, transform::TransformPlugin::kName};
    return kDependencies;
}

void TextPlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<FontAssets>();
    resources.getMut<FontAssets>()->setDirectory(assetDirectoryOf(resources));
    resources.insertIfAbsent<graphics::TextureAssets>();
    resources.insertIfAbsent<graphics::SpriteQueue>();
    static_cast<void>(app.getWorld().getComponents().registerType<TextRenderer>());
    addTextCalls(app);
    app.addSystem(ecs::Phase::kRender, std::string{kTexturesSystem}, &queueFontTextures)
        .before(std::string{graphics::GraphicsPlugin::kTexturesSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawTexts)
        .after(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .before(std::string{graphics::GraphicsPlugin::kSpritesSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::text
