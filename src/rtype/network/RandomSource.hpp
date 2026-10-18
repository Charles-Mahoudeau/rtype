/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RandomSource
*/

#pragma once

#include <cstdint>
#include <functional>

#include "rtype/network/Export.hpp"

namespace rtype::network {

/// @brief Where challenges and tokens come from; tests inject a fixed sequence.
using RandomSource = std::function<std::uint64_t()>;

/// @brief Builds a random source backed by `std::random_device`, never by a seeded pseudo-random generator.
[[nodiscard]] RTYPE_NETWORK_API RandomSource makeSystemRandom();

}  // namespace rtype::network
