/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Commands
*/

#include "Commands.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/ActorSpawner.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
Commands::EntityCommands::EntityCommands(Commands& commands, Entity entity) noexcept
    : _commands{&commands}, _entity{entity} {}

Entity Commands::EntityCommands::getEntity() const noexcept { return _entity; }

void Commands::EntityCommands::despawn() { _commands->despawn(_entity); }

Commands::Commands(World& world) noexcept : _world{&world} {}

Commands::~Commands() { clear(); }

Commands::Commands(Commands&& other) noexcept
    : _world{other._world},
      _records{std::move(other._records)},
      _columns{std::move(other._columns)},
      _reserved{std::move(other._reserved)} {
    other._records.clear();  // a moved-from vector is valid but unspecified: make `other` empty for sure
    other._columns.clear();
    other._reserved.clear();
}

Commands& Commands::operator=(Commands&& other) noexcept {
    if (this != &other) {
        clear();
        _world = other._world;
        _records = std::move(other._records);
        _columns = std::move(other._columns);
        _reserved = std::move(other._reserved);
        other._records.clear();
        other._columns.clear();
        other._reserved.clear();
    }
    return *this;
}

std::size_t Commands::getCount() const noexcept { return _records.size(); }

bool Commands::isEmpty() const noexcept { return _records.empty(); }

Commands::EntityCommands Commands::spawn() {
    const Entity entity = _world->reserve();
    try {
        _reserved.push_back(entity);
    } catch (...) {
        _world->despawn(entity);  // cancels the reservation this buffer could not track
        throw;
    }
    return EntityCommands{*this, entity};
}

namespace {
/// @brief Records each step of an actor's spawn in a Commands buffer.
class ActorRecorder {
  public:
    explicit ActorRecorder(Commands& commands) noexcept : _commands{&commands} {}
    ~ActorRecorder() = default;
    ActorRecorder(const ActorRecorder& other) = delete;
    ActorRecorder& operator=(const ActorRecorder& other) = delete;
    ActorRecorder(ActorRecorder&& other) = delete;
    ActorRecorder& operator=(ActorRecorder&& other) = delete;

    [[nodiscard]] Entity spawnEntity() { return _commands->spawn().getEntity(); }
    void addValue(Entity entity, ComponentId id, std::span<const std::byte> value) {
        static_cast<void>(_commands->add(entity, id, value));  // the id comes from the registry: always known
    }
    void setParent(Entity child, Entity parent) { _commands->add(child, Parent{.entity = parent}); }

  private:
    Commands* _commands;  ///< The buffer the steps are recorded in.
};
}  // namespace

Commands::EntityCommands Commands::spawnActor(ActorId id) {
    const Actor* actor = _world->getActors().findById(id);
    if (actor == nullptr) {
        throw exceptions::ActorException{std::format("No actor has the id {}", id)};
    }
    ActorRecorder recorder{*this};
    return entity(detail::spawnActorTree(_world->getActors(), _world->getComponents().getRegistry(), *actor, recorder));
}

Commands::EntityCommands Commands::spawnActor(std::string_view name) {
    const Actor* actor = _world->getActors().findByName(name);
    if (actor == nullptr) {
        throw exceptions::ActorException{std::format("No actor is named '{}'", name)};
    }
    return spawnActor(actor->getId());
}

Commands::EntityCommands Commands::entity(Entity entity) noexcept { return EntityCommands{*this, entity}; }

bool Commands::add(Entity entity, ComponentId id, std::span<const std::byte> value) {
    const std::optional<std::uint32_t> column = columnOf(id);
    if (!column) {
        return false;
    }
    storage::ByteColumn& values = std::next(_columns.begin(), *column)->values;
    if (value.size() != values.getElementSize()) {
        return false;
    }
    static_cast<void>(values.emplaceBackCopy(value));
    _records.push_back(Record{.kind = Kind::kAdd,
                              .entity = entity,
                              .column = *column,
                              .index = static_cast<std::uint32_t>(values.getSize() - 1)});
    return true;
}

bool Commands::remove(Entity entity, ComponentId id) {
    const std::optional<std::uint32_t> column = columnOf(id);
    if (!column) {
        return false;
    }
    _records.push_back(Record{.kind = Kind::kRemove, .entity = entity, .column = *column, .index = 0});
    return true;
}

void Commands::despawn(Entity entity) {
    _records.push_back(Record{.kind = Kind::kDespawn, .entity = entity, .column = 0, .index = 0});
}

void Commands::flush() {
    Components& components = _world->getComponents();
    for (Column& column : _columns) {  // registers first: if it throws, nothing is applied yet
        if (column.resolve != nullptr) {
            column.id = column.resolve(components);
            column.resolve = nullptr;
        }
    }
    _world->flushReserved();
    _reserved.clear();  // alive now: no longer this buffer's to cancel
    for (const Record& record : _records) {
        switch (record.kind) {
            case Kind::kAdd: {
                Column& column = *std::next(_columns.begin(), record.column);
                if (column.key == TypeTraits<Parent>::kLayoutHash) {
                    const Parent* parent = storage::viewAs<Parent>(column.values.at(record.index));
                    static_cast<void>(_world->setParent(record.entity, parent->entity));
                    break;
                }
                static_cast<void>(components.addMove(record.entity, column.id, column.values.at(record.index)));
                break;  // false for an entity dead by now: skipped
            }
            case Kind::kRemove: {
                const Column& column = *std::next(_columns.begin(), record.column);
                if (column.key == TypeTraits<Parent>::kLayoutHash) {
                    static_cast<void>(_world->removeParent(record.entity));
                    break;
                }
                static_cast<void>(components.remove(record.entity, column.id));
                break;
            }
            case Kind::kDespawn:
                static_cast<void>(_world->despawn(record.entity));
                break;
        }
    }
    _records.clear();
    for (Column& column : _columns) {
        column.values.clear();  // destroys the moved-from values, keeps the capacity
    }
}

void Commands::clear() noexcept {
    _records.clear();
    for (Column& column : _columns) {
        column.values.clear();
    }
    for (const Entity entity : _reserved) {
        static_cast<void>(_world->despawn(entity));  // cancels it, or despawns it if another flush made it alive
    }
    _reserved.clear();
}

std::optional<std::uint32_t> Commands::columnOf(ComponentId id) {
    const ComponentInfo* info = _world->getComponents().getRegistry().findById(id);
    if (info == nullptr) {
        return std::nullopt;
    }
    if (const std::optional<std::uint32_t> column = findColumn(info->type.layoutHash)) {
        return column;
    }
    _columns.push_back(
        Column{.key = info->type.layoutHash, .id = id, .resolve = nullptr, .values = storage::ByteColumn{info->type}});
    return static_cast<std::uint32_t>(_columns.size() - 1);
}

std::optional<std::uint32_t> Commands::findColumn(std::uint64_t key) const noexcept {
    const auto column = std::ranges::find(_columns, key, &Column::key);
    if (column == _columns.end()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(std::distance(_columns.begin(), column));
}
}  // namespace rtype::ecs
