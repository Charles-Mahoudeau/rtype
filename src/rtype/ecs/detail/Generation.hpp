/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Generation
*/

#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace rtype::ecs::detail {
/// @brief The generation a slot gets when it is released.
/// @param generation The slot's current generation.
/// @return generation + 1, or nothing when the generation is at its maximum: the slot must then be retired,
/// since wrapping around would let a very old handle match a new entity again.
[[nodiscard]] constexpr std::optional<std::uint32_t> nextGeneration(std::uint32_t generation) noexcept {
    if (generation == std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return generation + 1;
}
}  // namespace rtype::ecs::detail
