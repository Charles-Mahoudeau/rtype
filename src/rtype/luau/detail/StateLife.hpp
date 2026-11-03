/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StateLife
*/

#pragma once

#include <atomic>

namespace rtype::luau::detail {
/// @brief Tells whether a Lua state still lives: shared between the runtime that owns the state and the handles
/// (`Ref`) on its values, so a handle that outlives the runtime knows its state is closed and leaves it alone.
///
/// @details Created with the usertype registry of the state and marked dead when the registry is destroyed, which
/// is before the state is closed. The flag is atomic: a handle may be destroyed on another thread than the runtime.
class StateLife {
  public:
    StateLife() = default;
    ~StateLife() = default;
    StateLife(const StateLife& other) = delete;
    StateLife& operator=(const StateLife& other) = delete;
    StateLife(StateLife&& other) = delete;
    StateLife& operator=(StateLife&& other) = delete;

    /// @return False once the state is closed, or about to be.
    [[nodiscard]] bool isAlive() const noexcept { return _alive.load(std::memory_order_acquire); }

    /// @brief Records that the state is closed: from now on no handle touches it.
    void markDead() noexcept { _alive.store(false, std::memory_order_release); }

  private:
    std::atomic<bool> _alive{true};  ///< Whether the state is still open.
};
}  // namespace rtype::luau::detail
