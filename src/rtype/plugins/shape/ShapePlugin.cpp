/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePlugin
*/

#include "plugins/shape/ShapePlugin.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"  // IWYU pragma: keep
#include "plugins/shape/ShapeRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::shape {

namespace {
/// @return The shape @p shape draws for an entity at @p global.
contract::renderer::Shape shapeOf(const transform::GlobalTransform& global, const ShapeRenderer& shape) {
    const float cosine = std::cos(global.rotation);
    const float sine = std::sin(global.rotation);
    const glm::vec2 end = shape.end * global.scale;
    return contract::renderer::Shape{
        .kind = shape.kind,
        .position = global.position,
        .end = global.position + glm::vec2{(end.x * cosine) - (end.y * sine), (end.x * sine) + (end.y * cosine)},
        .size = shape.size * global.scale,
        .radius = shape.radius * global.scale.x,
        .halfHeight = shape.halfHeight * global.scale.x,
        .cornerRadius = shape.cornerRadius * global.scale.x,
        .rotation = global.rotation,
        .fillColor = shape.fillColor,
        .outlineColor = shape.outlineColor,
        .outlineThickness = shape.outlineThickness * global.scale.x,
        .blend = shape.blend};
}

/// @brief Render, client: draws the entities with a GlobalTransform and a ShapeRenderer, by layer, over the sprites.
void drawShapes(ecs::NonSendMut<plugins::graphics::Backend> backend,
                ecs::Query<const transform::GlobalTransform, const ShapeRenderer> shapes) {
    if (backend->renderer == nullptr) {
        return;
    }
    std::vector<std::pair<std::int32_t, contract::renderer::Shape>> drawn;
    for (auto [entity, global, shape] : shapes) {
        drawn.emplace_back(shape.layer, shapeOf(global, shape));
    }
    std::ranges::stable_sort(drawn, {}, &std::pair<std::int32_t, contract::renderer::Shape>::first);
    for (const auto& [layer, shape] : drawn) {
        backend->renderer->draw(shape);
    }
}
}  // namespace

std::string_view ShapePlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> ShapePlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 2> kDependencies{graphics::GraphicsPlugin::kName,
                                                                   transform::TransformPlugin::kName};
    return kDependencies;
}

void ShapePlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<ShapeRenderer>());
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawShapes)
        .after(std::string{graphics::GraphicsPlugin::kSpritesSystem})
        .before(std::string{graphics::GraphicsPlugin::kEndFrameSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::shape
