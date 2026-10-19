/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Phase
*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace rtype::ecs {
/// @brief A step of the frame: systems are registered in one phase, and the phases run in this order.
/// @details Startup runs once, before the first frame. Each frame then runs PreUpdate, FixedUpdate (zero or more
/// times: once per fixed tick), Update, PostUpdate and Render. The commands systems record are applied after each
/// phase. A dedicated server runs only PreUpdate, FixedUpdate and PostUpdate.
enum class Phase : std::uint8_t {
    kStartup,      ///< Once, before the first frame: setup, initial entities.
    kPreUpdate,    ///< Poll the window and input, receive network packets.
    kFixedUpdate,  ///< Gameplay at a fixed rate: Luau systems, physics, network send (server).
    kUpdate,       ///< Once per frame: animation, camera, UI.
    kPostUpdate,   ///< Transform propagation, input send (client).
    kRender,       ///< Extract what to draw, then draw.
};

/// @brief The number of phases.
inline constexpr std::size_t kPhaseCount = 6;
}  // namespace rtype::ecs
