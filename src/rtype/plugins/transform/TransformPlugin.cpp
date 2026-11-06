/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformPlugin
*/

#include "plugins/transform/TransformPlugin.hpp"

#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
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
void insertGlobalTransforms(
    ecs::Commands& commands,
    ecs::Query<const Transform, ecs::Optional<const ecs::Parent>, ecs::Without<GlobalTransform>> fresh,
    ecs::Query<const GlobalTransform> placed) {
    std::unordered_map<ecs::Entity, GlobalTransform> known;
    std::vector<std::tuple<ecs::Entity, const Transform*, ecs::Entity>> waiting;
    for (auto [entity, transform, parent] : fresh) {
        if (parent == nullptr) {
            known.emplace(entity, toGlobal(transform));
            commands.entity(entity).add(toGlobal(transform));
        } else {
            waiting.emplace_back(entity, &transform, parent->entity);
        }
    }
    if (waiting.empty()) {
        return;
    }
    for (auto [entity, global] : placed) {
        known.emplace(entity, global);
    }
    bool progressed = true;
    while (progressed && !waiting.empty()) {
        progressed = false;
        std::erase_if(waiting, [&](const auto& entry) {
            const auto& [entity, transform, parent] = entry;
            const auto found = known.find(parent);
            if (found == known.end()) {
                return false;
            }
            const GlobalTransform global = combine(found->second, *transform);
            known.emplace(entity, global);
            commands.entity(entity).add(global);
            progressed = true;
            return true;
        });
    }
    for (const auto& [entity, transform, parent] : waiting) {
        commands.entity(entity).add(toGlobal(*transform));
    }
}

/// @brief What the walk down a hierarchy needs of one entity.
struct Node {
    ecs::Mut<GlobalTransform> global;  ///< Written by the walk.
    const Transform* local;            ///< Relative to its parent.
    const ecs::Parent* parent;         ///< Null for a root.
    const ecs::Children* children;     ///< Null without children.
};

/// @brief A child to compute: the entity, and where its parent is in the world.
struct Step {
    ecs::Entity entity;
    GlobalTransform parent;
};

/// @brief Writes `value` only if it differs, so an entity that did not move is not marked as changed.
void update(ecs::Mut<GlobalTransform>& global, const GlobalTransform& value) {
    const GlobalTransform& current = global.get();
    if (current.position != value.position || current.rotation != value.rotation || current.scale != value.scale) {
        *global = value;
    }
}

/// @brief Queues the children of `node`, which sits at `global` in the world.
void pushChildren(const Node& node, const GlobalTransform& global, std::vector<Step>& steps) {
    if (node.children != nullptr) {
        for (const ecs::Entity child : node.children->entities) {
            steps.push_back(Step{.entity = child, .parent = global});
        }
    }
}

/// @brief Computes every GlobalTransform, from the roots down.
void propagateTransforms(
    ecs::Query<GlobalTransform, const Transform, ecs::Optional<const ecs::Parent>, ecs::Optional<const ecs::Children>>
        transforms) {
    std::unordered_map<ecs::Entity, Node> nodes;
    for (auto [entity, global, transform, parent, children] : transforms) {
        if (parent == nullptr && children == nullptr) {
            update(global, toGlobal(transform));
            continue;
        }
        nodes.emplace(entity, Node{.global = global, .local = &transform, .parent = parent, .children = children});
    }
    std::vector<Step> steps;
    for (auto& [entity, node] : nodes) {
        if (node.parent == nullptr || !nodes.contains(node.parent->entity)) {
            const GlobalTransform global = toGlobal(*node.local);
            update(node.global, global);
            pushChildren(node, global, steps);
        }
    }
    while (!steps.empty()) {
        const Step step = steps.back();
        steps.pop_back();
        const auto found = nodes.find(step.entity);
        if (found != nodes.end()) {
            const GlobalTransform global = combine(step.parent, *found->second.local);
            update(found->second.global, global);
            pushChildren(found->second, global, steps);
        }
    }
}

}  // namespace

std::string_view TransformPlugin::getName() const noexcept { return kName; }

void TransformPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<Transform>());
    static_cast<void>(app.getWorld().getComponents().registerType<GlobalTransform>());
    static_cast<void>(app.getWorld().getComponents().registerType<ecs::Parent>(ecs::kParentOptions));
    static_cast<void>(app.getWorld().getComponents().registerType<ecs::Children>());
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kInsertSystem}, &insertGlobalTransforms)
        .setSide(ecs::Side::kBoth);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kPropagateSystem}, &propagateTransforms)
        .after(std::string{kInsertSystem})
        .setSide(ecs::Side::kBoth);
}

}  // namespace rtype::plugins::transform
