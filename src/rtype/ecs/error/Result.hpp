/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Result
*/

#pragma once

#include <string>
#include <tl/expected.hpp>
#include <utility>

#include "Error.hpp"

namespace rtype::ecs {
/// @brief Outcome of an operation that may fail for a reason: either a value of type `T` or an Error.
///
/// @details Alias of `tl::expected<T, Error>` (to become `std::expected` once every target compiler supports it):
/// test it with its `bool` conversion, read the value with `*` / `->` and the failure with `error()`. Only used on
/// cold paths, such as registering a component; hot paths report failures with `bool`, `std::optional` or `nullptr`.
/// @tparam T Type of the success value; `void` for operations that only succeed or fail.
template <typename T = void>
using Result = tl::expected<T, Error>;

/// @brief Builds the error side of any Result.
/// @code
/// rtype::ecs::Result<void> check(int value) {
///     if (value < 0) {
///         return rtype::ecs::fail(rtype::ecs::ErrorKind::kInvalidComponent, "value must be positive");
///     }
///     return {};
/// }
/// @endcode
[[nodiscard]] inline tl::unexpected<Error> fail(ErrorKind kind, std::string message) {
    return tl::unexpected<Error>{Error{kind, std::move(message)}};
}
}  // namespace rtype::ecs
