/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeConfig
*/

#include "RuntimeConfig.hpp"

#include <utility>

namespace rtype::luau {
RuntimeConfig::Libs operator|(const RuntimeConfig::Libs lhs, const RuntimeConfig::Libs rhs) noexcept {
    return static_cast<RuntimeConfig::Libs>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

RuntimeConfig::Libs operator&(const RuntimeConfig::Libs lhs, const RuntimeConfig::Libs rhs) noexcept {
    return static_cast<RuntimeConfig::Libs>(std::to_underlying(lhs) & std::to_underlying(rhs));
}
}  // namespace rtype::luau
