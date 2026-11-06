/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include "World.hpp"

#include <cstddef>
#include <format>
#include <span>
#include <string_view>
#include <vector>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/ActorSpawner.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/call/Calls.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::ecs {

namespace {
/// @brief Applies each step of an actor's spawn to a World at once.
class ActorBuilder {
  public:
    explicit ActorBuilder(World& world) noexcept : _world{&world} {}
    ~ActorBuilder() = default;
    ActorBuilder(const ActorBuilder& other) = delete;
    ActorBuilder& operator=(const ActorBuilder& other) = delete;
    ActorBuilder(ActorBuilder&& other) = delete;
    ActorBuilder& operator=(ActorBuilder&& other) = delete;

    /// @return The entities spawned so far, to despawn them if the spawn fails halfway.
    [[nodiscard]] const std::vector<Entity>& getSpawned() const noexcept { return _spawned; }

    [[nodiscard]] Entity spawnEntity() { return _spawned.emplace_back(_world->spawn()); }
    void addValue(Entity entity, ComponentId id, std::span<const std::byte> value) {
        static_cast<void>(_world->getComponents().add(entity, id, value));  // alive, known id: cannot fail
    }
    void setParent(Entity child, Entity parent) { static_cast<void>(_world->setParent(child, parent)); }

  private:
    World* _world;                 ///< The world the steps are applied to.
    std::vector<Entity> _spawned;  ///< Every entity spawned, root first.
};

/// @brief Takes `child` out of its parent's Children, removing the component once it is empty.
void detachFromParent(Components& components, Entity child) noexcept {
    const auto* parent = components.get<Parent>(child);
    if (parent == nullptr) {
        return;
    }
    const Entity owner = parent->entity;
    if (auto children = components.getMut<Children>(owner); children) {
        std::erase(children->entities, child);
        if (children->entities.empty()) {
            static_cast<void>(components.remove<Children>(owner));
        }
    }
}

/// @brief Takes the deepest last descendant of `root` out of its parent's Children.
/// @return That descendant, or the null entity if `root` has no child left.
Entity takeDeepestChild(Components& components, Entity root) noexcept {
    Entity owner = root;
    while (true) {
        const auto* children = components.get<Children>(owner);
        if (children == nullptr || children->entities.empty()) {
            return Entity{};
        }
        const Entity last = children->entities.back();
        const auto* below = components.get<Children>(last);
        if (below == nullptr || below->entities.empty()) {
            components.getMut<Children>(owner)->entities.pop_back();
            return last;
        }
        owner = last;
    }
}

}  // namespace

Components& World::getComponents() noexcept { return _state->components; }

const Components& World::getComponents() const noexcept { return _state->components; }

Resources& World::getResources() noexcept { return _state->resources; }

const Resources& World::getResources() const noexcept { return _state->resources; }

Events& World::getEvents() noexcept { return _state->events; }

const Events& World::getEvents() const noexcept { return _state->events; }

Actors& World::getActors() noexcept { return _state->actors; }

const Actors& World::getActors() const noexcept { return _state->actors; }
Calls& World::getCalls() noexcept { return _state->calls; }

const Calls& World::getCalls() const noexcept { return _state->calls; }

Tick World::getChangeTick() const noexcept { return _state->changeTick; }

std::size_t World::getEntityCount() const noexcept { return _state->entities.getAliveCount(); }

Tick World::advanceChangeTick(Tick count) noexcept {
    _state->changeTick += count;  // wraps around after 2^32 ticks: comparisons and clamping are built for it
    if (_state->changeTick - _state->lastCheckTick >= kCheckTickThreshold) {
        checkChangeTicks();
    }
    return _state->changeTick;
}

void World::checkChangeTicks() noexcept {
    _state->components.clampTicks(_state->changeTick);
    _state->resources.clampTicks(_state->changeTick);
    _state->lastCheckTick = _state->changeTick;
}

Entity World::spawn() { return _state->entities.allocate(); }

Entity World::reserve() { return _state->entities.reserve(); }

void World::flushReserved() { _state->entities.flushReserved(); }

Entity World::spawnActor(ActorId id) {
    const Actor* actor = getActors().findById(id);
    if (actor == nullptr) {
        throw exceptions::ActorException{std::format("No actor has the id {}", id)};
    }
    ActorBuilder builder{*this};
    try {
        return detail::spawnActorTree(getActors(), getComponents().getRegistry(), *actor, builder);
    } catch (...) {
        for (const Entity entity : builder.getSpawned()) {  // no half-built tree is left behind
            static_cast<void>(despawn(entity));
        }
        throw;
    }
}

Entity World::spawnActor(std::string_view name) {
    const Actor* actor = getActors().findByName(name);
    if (actor == nullptr) {
        throw exceptions::ActorException{std::format("No actor is named '{}'", name)};
    }
    return spawnActor(actor->getId());
}

void World::destroyEntity(Entity entity) noexcept {
    detachFromParent(_state->components, entity);
    _state->components.removeAll(entity);
    _state->components.clearReferencesTo(entity);
    _state->entities.release(entity);
}

bool World::despawn(Entity entity) noexcept {
    if (_state->entities.isReserved(entity)) {
        return _state->entities.release(entity);  // cancels it: a reserved entity has no component yet
    }
    if (!isAlive(entity)) {
        return false;
    }
    for (Entity child = takeDeepestChild(_state->components, entity); child != Entity{};
         child = takeDeepestChild(_state->components, entity)) {
        if (isAlive(child)) {
            destroyEntity(child);
        }
    }
    destroyEntity(entity);
    return true;
}

bool World::setParent(Entity child, Entity parent) {
    if (!isAlive(child) || !isAlive(parent) || child == parent) {
        return false;
    }
    if (const auto* current = getComponents().get<Parent>(child); current != nullptr && current->entity == parent) {
        return true;
    }
    for (const auto* ancestor = getComponents().get<Parent>(parent); ancestor != nullptr;
         ancestor = getComponents().get<Parent>(ancestor->entity)) {
        if (ancestor->entity == child) {
            return false;
        }
    }
    detachFromParent(getComponents(), child);
    if (getComponents().getRegistry().find<Parent>() == nullptr) {
        static_cast<void>(getComponents().registerType<Parent>(kParentOptions));
    }
    static_cast<void>(getComponents().add(child, Parent{.entity = parent}));
    if (auto children = getComponents().getMut<Children>(parent); children) {
        children->entities.push_back(child);
    } else {
        static_cast<void>(getComponents().add(parent, Children{.entities = {child}}));
    }
    return true;
}

bool World::removeParent(Entity child) {
    if (!isAlive(child)) {
        return false;
    }
    if (getComponents().get<Parent>(child) == nullptr) {
        return false;
    }
    detachFromParent(getComponents(), child);
    static_cast<void>(getComponents().remove<Parent>(child));
    return true;
}

bool World::isAlive(Entity entity) const noexcept { return _state->entities.isAlive(entity); }
}  // namespace rtype::ecs
