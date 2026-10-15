/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Mut
*/

#pragma once

#include "rtype/ecs/change/Tick.hpp"

namespace rtype::ecs {
/// @brief Write access to one component value that records the write: marks the value changed when it is used to
/// modify it.
///
/// @details Reading through get() is free; `->` and `*` stamp the value's changed tick, so only the values a system
/// actually touches are seen as changed by queries and replication. A Mut does not own the value: it is
/// invalidated, like any address into a pool, by the next insertion into or removal from that pool.
/// @code
/// rtype::ecs::Mut<Health> health = world.getMut<Health>(ship);
/// if (health && health.get().value > 0) {  // reading: not marked
///     health->value -= 1;                    // writing: marked changed
/// }
/// @endcode
template <typename T>
class Mut {
  public:
    /// @brief A handle to a value, stamping `changed` with `tick` on write access.
    Mut(T* value, Tick* changed, Tick tick) noexcept;
    /// @brief An empty handle: the component is absent.
    Mut() noexcept = default;
    ~Mut() = default;
    Mut(const Mut& other) noexcept = default;
    Mut& operator=(const Mut& other) noexcept = default;
    Mut(Mut&& other) noexcept = default;
    Mut& operator=(Mut&& other) noexcept = default;

    /// @return The value, read-only: does not mark it changed.
    /// @note The handle must not be empty.
    [[nodiscard]] const T& get() const noexcept;

    /// @brief Write access: marks the value changed.
    /// @note The handle must not be empty.
    [[nodiscard]] T* operator->() noexcept;

    /// @brief Write access: marks the value changed.
    /// @note The handle must not be empty.
    [[nodiscard]] T& operator*() noexcept;

    /// @return False for an empty handle (the component was absent).
    [[nodiscard]] explicit operator bool() const noexcept;

  private:
    /// @brief Stamps the value's changed tick.
    void markChanged() noexcept;

    T* _value{nullptr};       ///< The value, in its pool; null for an empty handle.
    Tick* _changed{nullptr};  ///< The value's changed tick, in its pool.
    Tick _tick{0};            ///< Tick stamped on write access: the world's tick when the handle was made.
};
}  // namespace rtype::ecs

#include "Mut.tpp"
