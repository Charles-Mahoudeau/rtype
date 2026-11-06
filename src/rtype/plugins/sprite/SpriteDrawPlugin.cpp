/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteDrawPlugin
*/

#include "plugins/sprite/SpriteDrawPlugin.hpp"

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/SpriteQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::sprite {

namespace {
/// @brief Render, client: pushes the sprites whose texture is loaded to the SpriteQueue. A sprite whose material is
/// created is drawn with it, and with its MaterialInstance's values or else its shader's defaults.
void drawEntities(
    ecs::Res<graphics::TextureAssets> textures, ecs::Res<shader::ShaderHandles> handles,
    ecs::ResAppend<graphics::SpriteQueue> queue,
    ecs::Query<const transform::GlobalTransform, const SpriteRenderer, ecs::Optional<const shader::MaterialInstance>>
        sprites) {
    for (auto [entity, global, sprite, instance] : sprites) {
        const std::optional<contract::renderer::TextureId> texture = textures->getTexture(sprite.texture);
        if (!texture) {
            continue;
        }
        contract::renderer::Sprite drawn{.texture = *texture,
                                         .source = sprite.source,
                                         .position = global.position,
                                         .scale = global.scale,
                                         .origin = sprite.origin,
                                         .rotation = global.rotation,
                                         .color = sprite.color,
                                         .blend = sprite.blend};
        if (const shader::ShaderHandles::Material* material = handles->getMaterial(sprite.material)) {
            drawn.material = material->material;
            drawn.instanceValues = instance != nullptr ? instance->values : material->instanceDefaults;
        }
        queue->push(sprite.layer, drawn);
    }
}
}  // namespace

std::string_view SpriteDrawPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> SpriteDrawPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 3> kDependencies{SpritePlugin::kName, graphics::GraphicsPlugin::kName,
                                                                   shader::ShaderPlugin::kName};
    return kDependencies;
}

void SpriteDrawPlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawEntities)
        .after(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .before(std::string{graphics::GraphicsPlugin::kSpritesSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::sprite
