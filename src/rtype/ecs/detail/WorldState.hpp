/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldState
*/

#pragma once

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/EntityAllocator.hpp"
#include "rtype/ecs/resource/Resources.hpp"

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

    EntityAllocator entities;      ///< Entity indices and generations.
    Components components{*this};  ///< Component registry, pools and values.
    Resources resources{*this};    ///< Resource registry, slots and values.
    Tick changeTick{0};            ///< Current tick, stamped on every value added or written.
    Tick lastCheckTick{0};         ///< Change tick at the last World::checkChangeTicks().
};
}  // namespace rtype::ecs::detail
