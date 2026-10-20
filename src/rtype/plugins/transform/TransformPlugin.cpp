/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformPlugin
*/

#include "plugins/transform/TransformPlugin.hpp"

#include <string>
#include <string_view>

#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::transform {
namespace {
/// @return The GlobalTransform of an entity without a parent: its Transform.
GlobalTransform toGlobal(const Transform& transform) {
    return GlobalTransform{.position = transform.position, .rotation = transform.rotation, .scale = transform.scale};
}

/// @brief PostUpdate: gives a GlobalTransform to the entities that have a Transform but none yet.
void insertGlobalTransforms(ecs::Commands& commands,
                            ecs::Query<const Transform, ecs::Without<GlobalTransform>> transforms) {
    for (auto [entity, transform] : transforms) {
        commands.entity(entity).add(toGlobal(transform));
    }
}

/// @brief PostUpdate: copies each changed Transform into its GlobalTransform.
void propagateTransforms(ecs::Query<GlobalTransform, const Transform, ecs::Changed<Transform>> transforms) {
    for (auto [entity, global, transform] : transforms) {
        *global = toGlobal(transform);
    }
}
}  // namespace

std::string_view TransformPlugin::getName() const noexcept { return kName; }

void TransformPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<Transform>());
    static_cast<void>(app.getWorld().getComponents().registerType<GlobalTransform>());
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kInsertSystem}, &insertGlobalTransforms)
        .setSide(ecs::Side::kBoth);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kPropagateSystem}, &propagateTransforms)
        .after(std::string{kInsertSystem})
        .setSide(ecs::Side::kBoth);
}

}  // namespace rtype::plugins::transform
