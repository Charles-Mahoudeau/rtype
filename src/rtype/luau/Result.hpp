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

/// @brief Propagates the error of a failed `tl::expected` into an `expected` of another value type.
///
/// Lets a function return early with the error of a sub-operation whose value type differs from its own.
/// @pre `from` holds an error (`!from`); calling it on a successful result is undefined behavior.
/// @tparam DestT Value type of the returned `expected`.
/// @tparam SrcT Value type of `from`.
/// @param from The failed result whose error is forwarded.
/// @return An `expected<DestT, ErrT>` holding a copy of the error of `from`.
template <typename DestT, typename SrcT>
[[nodiscard]] Result<DestT> forwardError(Result<SrcT> from);
}  // namespace rtype::luau

// NOLINTNEXTLINE(*-include-cleaner)
#include "Result.tpp"
