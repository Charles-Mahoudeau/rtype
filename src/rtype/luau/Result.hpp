/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Result
*/

#pragma once

#include <expected>

#include "Error.hpp"

namespace rtype::luau {
template <typename T = void>
using Result = std::expected<T, Error>;
}  // namespace rtype::luau
