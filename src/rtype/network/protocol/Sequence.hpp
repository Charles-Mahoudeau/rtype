/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sequence
*/

#pragma once

#include <cstdint>

namespace rtype::network::protocol {
/// @brief Tells whether a sequence number is ahead of another, across the wrap from 65535 to 0.
///
/// @details Every comparison of a sequence number or a reliable message id goes through this function: a plain `<`
/// gives the wrong answer once the counter wraps. At exactly half the range (32768 apart) the order is undefined and
/// the result is false both ways.
/// @param a The number to test.
/// @param b The number to compare with.
/// @return True if `a` is ahead of `b` by 1 to 32767 steps, counting forward with wraparound.
[[nodiscard]] constexpr bool isNewer(std::uint16_t a, std::uint16_t b) noexcept {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(a - b)) > 0;
}
}  // namespace rtype::network::protocol
