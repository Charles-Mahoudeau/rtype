/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlayerInput
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::replication::component {
/// @brief Number of axis slots in a PlayerInput.
inline constexpr std::size_t kMaxInputAxes = 16;

/// @brief The input state that drives a controlled entity for one tick: which buttons are held and the value of each
/// axis.
/// @details Gameplay systems read it next to the entity's other components, whether it came from the network (on the
/// server), the local keyboard (standalone) or a replayed input (prediction). All zeros is the neutral input. It is
/// never sent in snapshots: inputs travel in their own message.
struct PlayerInput {
    std::uint32_t buttons{0};                 ///< Bit `i` set: button action `i` is held.
    std::uint32_t previousButtons{0};         ///< The buttons of the input applied before, to tell presses.
    std::array<float, kMaxInputAxes> axes{};  ///< The axis slots, each in [-1, 1].
    std::uint32_t sequence{0};                ///< The client input this one came from.
};

RTYPE_ECS_COMPONENT(PlayerInput, "net.PlayerInput", buttons, previousButtons, axes, sequence);
}  // namespace rtype::replication::component
