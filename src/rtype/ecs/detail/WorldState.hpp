/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldState
*/

#pragma once

#include <string_view>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/EntityAllocator.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

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

    /// @brief Checks that `name` is not taken by another kind of type: a name is one kind only, so the Luau bridge,
    /// the network manifest and save files can resolve it alone.
    /// @param kind The kind registering the name; its own registry is not checked (it handles re-registration).
    /// @return Nothing when the name is free, or an ErrorKind::kNameCollision error.
    [[nodiscard]] Result<void> checkNameFree(std::string_view name, TypeKind kind) const;

    EntityAllocator entities;      ///< Entity indices and generations.
    Components components{*this};  ///< Component registry, pools and values.
    Resources resources{*this};    ///< Resource registry, slots and values.
    Events events{*this};          ///< Event registry and queues.
    Tick changeTick{0};            ///< Current tick, stamped on every value added or written.
    Tick lastCheckTick{0};         ///< Change tick at the last World::checkChangeTicks().
};
}  // namespace rtype::ecs::detail
