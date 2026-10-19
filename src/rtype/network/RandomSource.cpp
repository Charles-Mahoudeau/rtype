/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RandomSource
*/

#include "rtype/network/RandomSource.hpp"

#include <cstdint>
#include <memory>
#include <random>

namespace rtype::network {

RandomSource makeSystemRandom() {
    // std::function needs a copyable callable, and std::random_device is not.
    const auto device = std::make_shared<std::random_device>();
    return [device] {
        const std::uint64_t high = (*device)();
        const std::uint64_t low = (*device)();
        return (high << 32U) | (low & 0xFFFFFFFFU);
    };
}

}  // namespace rtype::network
