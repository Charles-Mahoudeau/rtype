/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Version
*/

#pragma once

#include <string_view>

#include "Export.hpp"

namespace rtype::ecs {
/// @brief Version of the rtype-ecs library.
/// @return The version as `MAJOR.MINOR.PATCH`, e.g. "0.1.0".
[[nodiscard]] RTYPE_ECS_API std::string_view getVersion() noexcept;
}  // namespace rtype::ecs
