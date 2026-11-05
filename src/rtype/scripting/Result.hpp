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
#include "ErrorKind.hpp"

namespace rtype::scripting {
/// @brief Outcome of an operation that may fail for a reason: either a value of type `T` or an Error.
///
/// @details Alias of `tl::expected<T, Error>`: test it with its `bool` conversion, read the value with `*` / `->` and
/// the failure with `error()`.
/// @tparam T Type of the success value; `void` for operations that only succeed or fail.
template <typename T = void>
using Result = tl::expected<T, Error>;

/// @brief Builds the error side of any Result.
[[nodiscard]] inline tl::unexpected<Error> fail(const ErrorKind kind, std::string message) {
    return tl::unexpected{Error{kind, std::move(message)}};
}
}  // namespace rtype::scripting
