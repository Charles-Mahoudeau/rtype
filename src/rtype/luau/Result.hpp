/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Result
*/

#pragma once

#include <tl/expected.hpp>

#include "Error.hpp"

namespace rtype::luau {
template <typename T = void>
using Result = tl::expected<T, Error>;
}  // namespace rtype::luau
