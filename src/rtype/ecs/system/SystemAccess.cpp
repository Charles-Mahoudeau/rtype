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

void SystemAccess::addResource(TypeId id, bool write, bool append) { add(_resources, id, write, append); }

void SystemAccess::addEvent(TypeId id, bool write) { add(_events, id, write); }

void SystemAccess::requireMainThread() noexcept { _mainThread = true; }

void SystemAccess::merge(const SystemAccess& other) {
    for (const TypeAccess& access : other._components) {
        add(_components, access.id, access.write);
    }
    for (const TypeAccess& access : other._resources) {
        add(_resources, access.id, access.write, access.append);
    }
    for (const TypeAccess& access : other._events) {
        add(_events, access.id, access.write);
    }
    _mainThread = _mainThread || other._mainThread;
}

void SystemAccess::add(std::vector<TypeAccess>& accesses, TypeId id, bool write, bool append) {
    const auto known = std::ranges::find(accesses, id, &TypeAccess::id);
    if (known != accesses.end()) {
        known->write = known->write || write;
        known->append = known->append && append;
        return;
    }
    accesses.push_back(TypeAccess{.id = id, .write = write, .append = append});
}
}  // namespace rtype::ecs
