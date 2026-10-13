/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Tick
*/

#pragma once

#include <cstdint>

namespace rtype::ecs {
/// @brief A moment of the world, counted in system runs.
///
/// @details Pools stamp each value with the tick it was added at and the tick it was last written at; a system
/// compares those ticks with the tick of its own last run to know what was added or changed since.
using Tick = std::uint32_t;
}  // namespace rtype::ecs
