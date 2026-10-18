/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemAccess
*/

#include "SystemAccess.hpp"

#include <algorithm>
#include <span>
#include <vector>

#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
std::span<const TypeAccess> SystemAccess::getComponents() const noexcept { return _components; }

std::span<const TypeAccess> SystemAccess::getResources() const noexcept { return _resources; }

std::span<const TypeAccess> SystemAccess::getEvents() const noexcept { return _events; }

bool SystemAccess::isMainThread() const noexcept { return _mainThread; }

void SystemAccess::addComponent(TypeId id, bool write) { add(_components, id, write); }

void SystemAccess::addResource(TypeId id, bool write) { add(_resources, id, write); }

void SystemAccess::addEvent(TypeId id, bool write) { add(_events, id, write); }

void SystemAccess::requireMainThread() noexcept { _mainThread = true; }

void SystemAccess::add(std::vector<TypeAccess>& accesses, TypeId id, bool write) {
    const auto known = std::ranges::find(accesses, id, &TypeAccess::id);
    if (known != accesses.end()) {
        known->write = known->write || write;
        return;
    }
    accesses.push_back(TypeAccess{.id = id, .write = write});
}
}  // namespace rtype::ecs
