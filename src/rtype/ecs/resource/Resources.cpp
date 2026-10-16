/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Resources
*/

#include "Resources.hpp"

#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
Resources::Resources(const detail::WorldState& state) noexcept : _state{&state} {}

const ResourceRegistry& Resources::getRegistry() const noexcept { return _registry; }

Result<ResourceId> Resources::registerType(ResourceInfo info) {
    if (Result<void> free = checkNameFree(info.type.name); !free) {
        return tl::unexpected<Error>{std::move(free.error())};
    }
    Result<ResourceId> id = _registry.registerType(std::move(info));
    if (id) {
        createStorages(*id);
    }
    return id;
}

std::optional<ResourceId> Resources::find(std::string_view name) const noexcept {
    if (const ResourceInfo* info = _registry.findByName(name)) {
        return info->type.id;
    }
    return std::nullopt;
}

bool Resources::insert(ResourceId id, std::span<const std::byte> value) {
    storage::ResourceStorage* resource = findStorage(id);
    if (resource == nullptr || value.size() != resource->getInfo().type.size || !resource->getInfo().type.copyable) {
        return false;
    }
    resource->insertCopy(value, getChangeTick());
    return true;
}

bool Resources::has(ResourceId id) const noexcept {
    const storage::ResourceStorage* resource = findStorage(id);
    return resource != nullptr && resource->hasValue();
}

std::span<const std::byte> Resources::get(ResourceId id) const noexcept {
    const storage::ResourceStorage* resource = findStorage(id);
    return resource != nullptr ? resource->get() : std::span<const std::byte>{};
}

std::span<std::byte> Resources::getMut(ResourceId id) noexcept {
    storage::ResourceStorage* resource = findStorage(id);
    if (resource == nullptr || !resource->hasValue()) {
        return {};
    }
    resource->getTicks().changed = getChangeTick();
    return resource->get();
}

bool Resources::remove(ResourceId id) noexcept {
    storage::ResourceStorage* resource = findStorage(id);
    return resource != nullptr && resource->remove();
}

std::optional<ComponentTicks> Resources::getTicks(ResourceId id) const noexcept {
    const storage::ResourceStorage* resource = findStorage(id);
    if (resource == nullptr || !resource->hasValue()) {
        return std::nullopt;
    }
    return resource->getTicks();
}

void Resources::clampTicks(Tick now) noexcept {
    for (storage::ResourceStorage& resource : _storages) {
        resource.clampTicks(now);
    }
}

Tick Resources::getChangeTick() const noexcept { return _state->changeTick; }

Result<void> Resources::checkNameFree(std::string_view name) const {
    return _state->checkNameFree(name, TypeKind::kResource);
}

void Resources::createStorages(ResourceId id) {
    // A loop, not a single push: if creating a slot ever threw, the next registration catches up.
    while (_storages.size() <= id) {
        _storages.emplace_back(*_registry.findById(static_cast<ResourceId>(_storages.size())));
    }
}

storage::ResourceStorage* Resources::findStorage(ResourceId id) noexcept {
    return id < _storages.size() ? &*std::next(_storages.begin(), id) : nullptr;
}

const storage::ResourceStorage* Resources::findStorage(ResourceId id) const noexcept {
    return id < _storages.size() ? &*std::next(_storages.begin(), id) : nullptr;
}
}  // namespace rtype::ecs
