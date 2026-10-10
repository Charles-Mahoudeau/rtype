/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContext
*/

#pragma once

namespace rtype::luau {
/// @brief Context of a single call to a Function, given to its handler.
///
/// Holds nothing yet. Move-only: a context belongs to one call.
class CallContext {
  public:
    /// @brief Creates an empty context.
    CallContext() = default;

    /// @brief Destroys the context.
    ~CallContext() = default;

    /// @brief Copy is deleted: a context belongs to a single call.
    CallContext(const CallContext&) = delete;

    /// @brief Copy is deleted: a context belongs to a single call.
    CallContext& operator=(const CallContext&) = delete;

    /// @brief Takes over another context.
    CallContext(CallContext&&) = default;

    /// @brief Takes over another context.
    /// @return A reference to this context.
    CallContext& operator=(CallContext&&) = default;
};
}  // namespace rtype::luau
