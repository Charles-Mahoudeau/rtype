/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicResources
*/

#include "DynamicResources.hpp"

#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>

#include "rtype/ecs/exceptions/ResourceExceptions.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
DynamicResources::DynamicResources(Resources& resources, std::span<const ResourceUse> uses) noexcept
    : _resources{&resources}, _uses{uses} {}

std::size_t DynamicResources::getCount() const noexcept { return _uses.size(); }

const ResourceUse& DynamicResources::getUse(const std::size_t index) const noexcept {
    assert(index < _uses.size());
    return *std::next(_uses.begin(), static_cast<std::ptrdiff_t>(index));
}

bool DynamicResources::has(const std::size_t index) const noexcept { return _resources->has(getUse(index).id); }

std::span<const std::byte> DynamicResources::get(const std::size_t index) const noexcept {
    return std::as_const(*_resources).get(getUse(index).id);
}

std::span<std::byte> DynamicResources::getMut(const std::size_t index) noexcept {
    const ResourceUse& use = getUse(index);
    return use.write ? _resources->getMut(use.id) : std::span<std::byte>{};
}

SystemParam<DynamicResources>::State SystemParam<DynamicResources>::init(World& world, Config config) {
    for (const ResourceUse& use : config.resources) {
        const ResourceInfo* info = world.getResources().getRegistry().findById(use.id);
        if (info == nullptr) {
            throw exceptions::ResourceRegistrationException{
                std::format("A resource of the system (id {}) is not registered", use.id)};
        }
        if (info->nonSend) {
            throw exceptions::ResourceRegistrationException{
                std::format("Resource '{}' is main-thread only: a dynamic system cannot use it", info->type.name)};
        }
    }
    return config;
}

DynamicResources SystemParam<DynamicResources>::get(State& state, const SystemContext& context) noexcept {
    return DynamicResources{context.world->getResources(), state.resources};
}

void SystemParam<DynamicResources>::addAccess(const State& state, SystemAccess& access) {
    for (const ResourceUse& use : state.resources) {
        access.addResource(use.id, use.write);
    }
}
}  // namespace rtype::ecs
