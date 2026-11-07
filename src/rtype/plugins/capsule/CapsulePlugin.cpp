/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsulePlugin
*/

#include "CapsulePlugin.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "plugins/AssetDirectory.hpp"
#include "plugins/HotReload.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleCalls.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsuleReloadable.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/ReloadedCapsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/core/reload/HotReloader.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::capsule {
namespace {
/// @brief A spawned root as the hot reload sees it.
struct Root {
    CapsuleInstance instance;           ///< Its capsule.
    std::vector<ecs::Entity> children;  ///< Its children now.
};

/// @return The nearest ancestor of `entity` that is a capsule's root, or the null entity.
ecs::Entity rootAbove(ecs::Entity entity, const std::unordered_map<ecs::Entity, Root>& roots,
                      const std::unordered_map<ecs::Entity, ecs::Entity>& parents) {
    for (auto parent = parents.find(entity); parent != parents.end(); parent = parents.find(parent->second)) {
        if (roots.contains(parent->second)) {
            return parent->second;
        }
    }
    return ecs::Entity{};
}

/// @return The outermost capsule root `entity` belongs to: up through the capsules that spawned it as a node, so their
/// nodes' values stay on it when it is reloaded.
ecs::Entity outermostRoot(ecs::Entity entity, const std::unordered_map<ecs::Entity, Root>& roots,
                          const std::unordered_map<ecs::Entity, ecs::Entity>& parents) {
    while (roots.at(entity).instance.nested) {
        const ecs::Entity above = rootAbove(entity, roots, parents);
        if (above == ecs::Entity{}) {
            break;
        }
        entity = above;
    }
    return entity;
}

/// @brief Render, with hot reload: reloads what was spawned from the capsules the CapsuleReloadable read again, nested
/// roots through the outermost capsule they belong to.
void reloadCapsules(ecs::Commands& commands, ecs::Res<Capsules> capsules, ecs::ResMut<ReloadedCapsules> reloaded,
                    ecs::Query<const CapsuleInstance, ecs::Optional<const ecs::Children>> roots,
                    ecs::Query<const ecs::Parent> parents) {
    if (reloaded->capsules.empty()) {
        return;
    }
    const std::vector<CapsuleId> reread = std::exchange(reloaded->capsules, {});
    std::unordered_map<ecs::Entity, ecs::Entity> parentOf;
    for (auto [entity, parent] : parents) {
        parentOf.emplace(entity, parent.entity);
    }
    std::unordered_map<ecs::Entity, Root> rootOf;
    for (auto [entity, instance, children] : roots) {
        rootOf.emplace(entity, Root{.instance = instance,
                                    .children = children != nullptr ? children->entities : std::vector<ecs::Entity>{}});
    }
    std::vector<ecs::Entity> outermost;
    for (const auto& [entity, root] : rootOf) {
        if (std::ranges::find(reread, root.instance.capsule) != reread.end()) {
            const ecs::Entity top = outermostRoot(entity, rootOf, parentOf);
            if (std::ranges::find(outermost, top) == outermost.end()) {
                outermost.push_back(top);
            }
        }
    }
    for (const ecs::Entity entity : outermost) {
        const Root& root = rootOf.at(entity);
        try {
            reloadCapsule(commands, *capsules, entity, root.instance, root.children);
        } catch (const exceptions::CapsuleException& error) {  // a file instancing a capsule that is gone
            std::cerr << std::format("[capsule] entity {} not reloaded: {}\n", entity.getIndex(), error.what());
        }
    }
}
}  // namespace

std::string_view CapsulePlugin::getName() const noexcept { return kName; }

void CapsulePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<Capsules>();
    resources.requireMut<Capsules>()->setRunMode(app.getSchedules().getRunMode());  // a spawn keeps this side's
    resources.insertIfAbsent<LoadedCapsule>();
    ecs::Components& components = app.getWorld().getComponents();
    if (components.getRegistry().find<CapsuleInstance>() == nullptr) {
        static_cast<void>(components.registerType<CapsuleInstance>());
    }
    addCapsuleCalls(app);
    const std::filesystem::path directory = assetDirectoryOf(resources);
    if (directory.empty()) {
        return;
    }
    resources.insert(CapsuleFiles{directory, components});
    core::reload::HotReloader* reloader = hotReloaderOf(resources);
    if (reloader == nullptr) {
        return;
    }
    // The reloader's paths are relative to the game; the assets' directory is a folder of it.
    const std::filesystem::path assets = directory.lexically_relative(reloader->root());
    if (assets.empty() || *assets.begin() == "..") {
        return;
    }
    resources.insertIfAbsent<ReloadedCapsules>();
    reloader->add(std::make_unique<CapsuleReloadable>(&*resources.getMut<CapsuleFiles>(),
                                                      &*resources.requireMut<Capsules>(),
                                                      &*resources.requireMut<ReloadedCapsules>(), assets));
    app.addSystem(ecs::Phase::kRender, std::string{kHotReloadSystem}, &reloadCapsules).setSide(ecs::Side::kBoth);
}

void CapsulePlugin::finish(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    ecs::Mut<CapsuleFiles> files = resources.getMut<CapsuleFiles>();
    if (files) {
        static_cast<void>(files->loadAll(*resources.requireMut<Capsules>()));
    }
}
}  // namespace rtype::plugins::capsule
