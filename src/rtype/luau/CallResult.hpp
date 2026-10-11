/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallResult
*/

#pragma once

#include "Export.hpp"

namespace rtype::luau {
/// @brief Result of a single call to a Function, returned by its handler and by `Function::call`.
///
/// Holds no values yet. Move-only: a result belongs to one call.
class RTYPE_LUAU_API CallResult {
  public:
    /// @brief Creates an empty result.
    CallResult() = default;

    /// @brief Destroys the result.
    ~CallResult() = default;

    /// @brief Copy is deleted: a result belongs to a single call.
    CallResult(const CallResult&) = delete;

    /// @brief Copy is deleted: a result belongs to a single call.
    CallResult& operator=(const CallResult&) = delete;

    /// @brief Takes over another result.
    CallResult(CallResult&&) = default;

    /// @brief Takes over another result.
    /// @return A reference to this result.
    CallResult& operator=(CallResult&&) = default;
};
}  // namespace rtype::luau
