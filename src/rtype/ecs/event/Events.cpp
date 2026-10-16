/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Events
*/

#include "Events.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/event/EventRegistry.hpp"
#include "rtype/ecs/storage/EventStorage.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
Events::Events(const detail::WorldState& state) noexcept : _state{&state} {}

const EventRegistry& Events::getRegistry() const noexcept { return _registry; }

Result<EventId> Events::registerType(EventInfo info) {
    if (Result<void> free = checkNameFree(info.type.name); !free) {
        return tl::unexpected<Error>{std::move(free.error())};
    }
    Result<EventId> id = _registry.registerType(std::move(info));
    if (id) {
        createStorages(*id);
    }
    return id;
}

std::optional<EventId> Events::find(std::string_view name) const noexcept {
    if (const EventInfo* info = _registry.findByName(name)) {
        return info->type.id;
    }
    return std::nullopt;
}

bool Events::send(EventId id, std::span<const std::byte> value) {
    storage::EventStorage* queue = findStorage(id);
    if (queue == nullptr || value.size() != queue->getInfo().type.size || !queue->getInfo().type.copyable) {
        return false;
    }
    queue->sendCopy(value);
    return true;
}

RawEventRange Events::read(EventId id, EventCursor& cursor) const noexcept {
    const storage::EventStorage* queue = findStorage(id);
    if (queue == nullptr) {
        return {};
    }
    const EventSequence first = std::clamp(cursor.next, queue->getFirst(), queue->getEnd());
    const EventSequence last = queue->getEnd();
    cursor.next = last;
    return RawEventRange{queue, first, last};
}

std::size_t Events::getCount(EventId id) const noexcept {
    const storage::EventStorage* queue = findStorage(id);
    return queue != nullptr ? queue->getCount() : 0;
}

bool Events::update(EventId id) noexcept {
    storage::EventStorage* queue = findStorage(id);
    if (queue == nullptr) {
        return false;
    }
    queue->update();
    return true;
}

bool Events::clear(EventId id) noexcept {
    storage::EventStorage* queue = findStorage(id);
    if (queue == nullptr) {
        return false;
    }
    queue->clear();
    return true;
}

void Events::update() noexcept {
    for (storage::EventStorage& queue : _storages) {
        queue.update();
    }
}

Result<void> Events::checkNameFree(std::string_view name) const {
    return _state->checkNameFree(name, TypeKind::kEvent);
}

void Events::createStorages(EventId id) {
    while (_storages.size() <= id) {
        _storages.emplace_back(*_registry.findById(static_cast<EventId>(_storages.size())));
    }
}

storage::EventStorage* Events::findStorage(EventId id) noexcept {
    return id < _storages.size() ? &*std::next(_storages.begin(), id) : nullptr;
}

const storage::EventStorage* Events::findStorage(EventId id) const noexcept {
    return id < _storages.size() ? &*std::next(_storages.begin(), id) : nullptr;
}
}  // namespace rtype::ecs
