/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawn
*/

#include "CapsuleSpawn.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/detail/CapsuleSpawner.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryState.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::plugins::capsule {
namespace {
/// @brief Records each step of a spawn in a Commands buffer.
class CapsuleRecorder {
  public:
    explicit CapsuleRecorder(ecs::Commands& commands) noexcept : _commands{&commands} {}
    ~CapsuleRecorder() = default;
    CapsuleRecorder(const CapsuleRecorder& other) = delete;
    CapsuleRecorder& operator=(const CapsuleRecorder& other) = delete;
    CapsuleRecorder(CapsuleRecorder&& other) = delete;
    CapsuleRecorder& operator=(CapsuleRecorder&& other) = delete;

    [[nodiscard]] ecs::Entity spawnEntity() { return _commands->spawn().getEntity(); }
    void addValue(ecs::Entity entity, ecs::ComponentId id, std::span<const std::byte> value) {
        static_cast<void>(_commands->add(entity, id, value));  // the id was registered when the value was set
    }
    void setParent(ecs::Entity child, ecs::Entity parent) { _commands->add(child, ecs::Parent{.entity = parent}); }
    void addInstance(ecs::Entity entity, CapsuleInstance instance) { _commands->add(entity, instance); }

  private:
    ecs::Commands* _commands;  ///< The buffer the steps are recorded in.
};

/// @brief Applies each step of a spawn to a World at once, keeping what it spawned to undo a failed spawn.
class CapsuleBuilder {
  public:
    explicit CapsuleBuilder(ecs::World& world) noexcept : _world{&world} {}
    ~CapsuleBuilder() = default;
    CapsuleBuilder(const CapsuleBuilder& other) = delete;
    CapsuleBuilder& operator=(const CapsuleBuilder& other) = delete;
    CapsuleBuilder(CapsuleBuilder&& other) = delete;
    CapsuleBuilder& operator=(CapsuleBuilder&& other) = delete;

    [[nodiscard]] ecs::Entity spawnEntity() { return _spawned.emplace_back(_world->spawn()); }
    void addValue(ecs::Entity entity, ecs::ComponentId id, std::span<const std::byte> value) {
        static_cast<void>(_world->getComponents().add(entity, id, value));  // alive, known id: cannot fail
    }
    void setParent(ecs::Entity child, ecs::Entity parent) { static_cast<void>(_world->setParent(child, parent)); }
    void addInstance(ecs::Entity entity, CapsuleInstance instance) {
        static_cast<void>(_world->getComponents().add(entity, instance));
    }

    /// @brief Despawns every entity spawned so far, last first: no half-built tree is left behind.
    void undo() noexcept {
        for (const ecs::Entity entity : std::views::reverse(_spawned)) {
            static_cast<void>(_world->despawn(entity));
        }
    }

  private:
    ecs::World* _world;                 ///< The world the steps are applied to.
    std::vector<ecs::Entity> _spawned;  ///< Every entity spawned, in order.
};

/// @return The capsule with this id.
/// @throws exceptions::CapsuleException When no capsule has it.
const Capsule& requireCapsule(const Capsules& capsules, CapsuleId id) {
    const Capsule* capsule = capsules.find(id);
    if (capsule == nullptr) {
        throw exceptions::CapsuleException{std::format("No capsule has the id {:#x}", id.value)};
    }
    return *capsule;
}

/// @return The capsule at `path`.
/// @throws exceptions::CapsuleException When no capsule is there.
const Capsule& requireCapsule(const Capsules& capsules, std::string_view path) {
    const Capsule* capsule = capsules.find(path);
    if (capsule == nullptr) {
        throw exceptions::CapsuleException{std::format("No capsule at '{}'", path)};
    }
    return *capsule;
}

/// @return The world's capsules.
/// @throws exceptions::CapsuleException When the world has none: the CapsulePlugin was not added.
const Capsules& requireCapsules(const ecs::World& world) {
    const auto* capsules = world.getResources().get<Capsules>();
    if (capsules == nullptr) {
        throw exceptions::CapsuleException{
            std::format("The world has no capsules: add the CapsulePlugin ('{}') first", CapsulePlugin::kName)};
    }
    return *capsules;
}

/// @brief Records the spawn of `capsule`, checked first.
ecs::Commands::EntityCommands record(ecs::Commands& commands, const Capsules& capsules, const Capsule& capsule) {
    detail::checkCapsule(capsules, capsule);  // before anything is recorded
    CapsuleRecorder recorder{commands};
    return commands.entity(detail::spawnTree(capsules, capsule, recorder, ecs::Entity{}, false));
}

/// @brief Spawns `capsule` on the World, checked first, into `root` (or a new entity), undoing a failed spawn.
ecs::Entity build(ecs::World& world, const Capsules& capsules, const Capsule& capsule, ecs::Entity root, bool nested) {
    CapsuleBuilder builder{world};
    try {
        return detail::spawnTree(capsules, capsule, builder, root, nested);
    } catch (...) {
        builder.undo();
        throw;
    }
}

/// @brief Records switching `loaded` to `capsule`.
ecs::Commands::EntityCommands recordLoad(ecs::Commands& commands, const Capsules& capsules, LoadedCapsule& loaded,
                                         const Capsule& capsule) {
    detail::checkCapsule(capsules, capsule);  // before the old capsule goes
    if (loaded.root != ecs::Entity{}) {
        commands.despawn(loaded.root);  // does nothing at the flush if the game despawned it already
    }
    ecs::Commands::EntityCommands spawned = record(commands, capsules, capsule);
    loaded = LoadedCapsule{.root = spawned.getEntity(), .capsule = capsule.getId()};
    return spawned;
}

/// @brief Switches the world's LoadedCapsule to `capsule` at once.
ecs::Entity buildLoad(ecs::World& world, const Capsule& capsule) {
    const Capsules& capsules = requireCapsules(world);
    detail::checkCapsule(capsules, capsule);
    ecs::Mut<LoadedCapsule> loaded = world.getResources().getMut<LoadedCapsule>();
    if (!loaded) {
        throw exceptions::CapsuleException{
            std::format("The world has no LoadedCapsule: add the CapsulePlugin ('{}') first", CapsulePlugin::kName)};
    }
    if (loaded->root != ecs::Entity{}) {
        static_cast<void>(world.despawn(loaded->root));  // false if the game despawned it already
    }
    const ecs::Entity root = build(world, capsules, capsule, ecs::Entity{}, false);
    *loaded = LoadedCapsule{.root = root, .capsule = capsule.getId()};
    return root;
}

/// @return The outermost capsule root `root` belongs to: itself unless another capsule spawned it as a node.
ecs::Entity outermostRoot(const ecs::Components& components, ecs::Entity root) {
    for (;;) {
        const auto* instance = components.get<CapsuleInstance>(root);
        if (instance == nullptr || !instance->nested) {
            return root;
        }
        ecs::Entity ancestor = root;
        bool found = false;
        while (const auto* parent = components.get<ecs::Parent>(ancestor)) {  // the nearest capsule root above
            ancestor = parent->entity;
            if (components.has<CapsuleInstance>(ancestor)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return root;
        }
        root = ancestor;
    }
}
}  // namespace

ecs::Commands::EntityCommands spawnCapsule(ecs::Commands& commands, const Capsules& capsules, CapsuleId id) {
    return record(commands, capsules, requireCapsule(capsules, id));
}

ecs::Commands::EntityCommands spawnCapsule(ecs::Commands& commands, const Capsules& capsules, std::string_view path) {
    return record(commands, capsules, requireCapsule(capsules, path));
}

ecs::Entity spawnCapsule(ecs::World& world, CapsuleId id) {
    const Capsules& capsules = requireCapsules(world);
    const Capsule& capsule = requireCapsule(capsules, id);
    detail::checkCapsule(capsules, capsule);
    return build(world, capsules, capsule, ecs::Entity{}, false);
}

ecs::Entity spawnCapsule(ecs::World& world, std::string_view path) {
    return spawnCapsule(world, requireCapsule(requireCapsules(world), path).getId());
}

ecs::Commands::EntityCommands loadCapsule(ecs::Commands& commands, const Capsules& capsules, LoadedCapsule& loaded,
                                          CapsuleId id) {
    return recordLoad(commands, capsules, loaded, requireCapsule(capsules, id));
}

ecs::Commands::EntityCommands loadCapsule(ecs::Commands& commands, const Capsules& capsules, LoadedCapsule& loaded,
                                          std::string_view path) {
    return recordLoad(commands, capsules, loaded, requireCapsule(capsules, path));
}

ecs::Entity loadCapsule(ecs::World& world, CapsuleId id) {
    return buildLoad(world, requireCapsule(requireCapsules(world), id));
}

ecs::Entity loadCapsule(ecs::World& world, std::string_view path) {
    return buildLoad(world, requireCapsule(requireCapsules(world), path));
}

void reloadCapsule(ecs::World& world, ecs::Entity root) {
    const auto* instance = world.getComponents().get<CapsuleInstance>(root);
    if (instance == nullptr) {
        throw exceptions::CapsuleException{
            std::format("Entity {} is not the root of a capsule: it has no CapsuleInstance", root.getIndex())};
    }
    const bool nested = instance->nested;
    const Capsules& capsules = requireCapsules(world);
    const Capsule& capsule = requireCapsule(capsules, instance->capsule);
    detail::checkCapsule(capsules, capsule);  // before the old children go
    if (const auto* children = world.getComponents().get<ecs::Children>(root)) {
        const std::vector<ecs::Entity> old = children->entities;  // the component goes with the last child
        for (const ecs::Entity child : old) {
            static_cast<void>(world.despawn(child));
        }
    }
    static_cast<void>(build(world, capsules, capsule, root, nested));
}

void reloadCapsule(ecs::Commands& commands, const Capsules& capsules, ecs::Entity root, const CapsuleInstance& instance,
                   std::span<const ecs::Entity> children) {
    const Capsule& capsule = requireCapsule(capsules, instance.capsule);
    detail::checkCapsule(capsules, capsule);  // before anything is recorded
    for (const ecs::Entity child : children) {
        commands.despawn(child);
    }
    CapsuleRecorder recorder{commands};
    static_cast<void>(detail::spawnTree(capsules, capsule, recorder, root, instance.nested));
}

std::size_t reloadInstances(ecs::World& world, CapsuleId id) {
    std::vector<ecs::Entity> roots;
    {
        const ecs::QueryState<const CapsuleInstance> state{world.getComponents()};
        ecs::Query<const CapsuleInstance> query{state, 0};
        for (auto [entity, instance] : query) {
            if (instance.capsule == id) {
                const ecs::Entity outermost = outermostRoot(world.getComponents(), entity);
                if (std::ranges::find(roots, outermost) == roots.end()) {
                    roots.push_back(outermost);
                }
            }
        }
    }
    std::size_t reloaded = 0;
    for (const ecs::Entity root : roots) {
        if (world.isAlive(root)) {  // gone if the game had parented it under another root reloaded before
            reloadCapsule(world, root);
            ++reloaded;
        }
    }
    return reloaded;
}
}  // namespace rtype::plugins::capsule
