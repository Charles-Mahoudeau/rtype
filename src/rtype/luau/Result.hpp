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
/// @brief Outcome of an operation: either a value of type `T` or an `Error`.
///
/// Alias of `tl::expected<T, Error>`, so `has_value()`, `value()`, `error()` and `operator*` are available. Use
/// `Failure` to build the error side without spelling the type.
/// @tparam T Type of the success value; `void` for operations that only report success or failure.
template <typename T = void>
using Result = tl::expected<T, Error>;
}  // namespace rtype::luau
