/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldState
*/

#pragma once

#include <deque>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/EntityAllocator.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"

namespace rtype::ecs::detail {
/// @brief Everything a World owns, kept on the heap behind one pointer.
/// @details Its address never changes, even when the World is moved, so the parts of the world can keep a pointer
/// to it. It is neither copyable nor movable for the same reason.
struct WorldState {
    WorldState() = default;
    ~WorldState() = default;
    WorldState(const WorldState& other) = delete;
    WorldState& operator=(const WorldState& other) = delete;
    WorldState(WorldState&& other) = delete;
    WorldState& operator=(WorldState&& other) = delete;

    EntityAllocator entities;           ///< Entity indices and generations.
    Components components{*this};       ///< Component registry, pools and values.
    ResourceRegistry resourceRegistry;  ///< Resources of the world, by id and name.
    /// @brief One slot per resource, indexed by ResourceId; a deque keeps their addresses, so a Mut stays valid
    /// when other resources are registered.
    std::deque<storage::ResourceStorage> resources;
    Tick changeTick{0};     ///< Current tick, stamped on every value added or written.
    Tick lastCheckTick{0};  ///< Change tick at the last World::checkChangeTicks().
};
}  // namespace rtype::ecs::detail
