/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FakeClock
*/

#pragma once

#include <chrono>

#include "rtype/network/Types.hpp"

namespace rtype::network::tests {
/// @brief A clock that only moves when the test calls `advance`, so time-dependent code runs instantly and
/// identically on every run.
class FakeClock {
  public:
    FakeClock() = default;
    ~FakeClock() = default;
    FakeClock(const FakeClock&) = default;
    FakeClock& operator=(const FakeClock&) = default;
    FakeClock(FakeClock&&) noexcept = default;
    FakeClock& operator=(FakeClock&&) noexcept = default;

    /// @return The current fake time.
    [[nodiscard]] TimePoint now() const noexcept { return _now; }

    /// @brief Moves the fake time forward by `duration`.
    void advance(const std::chrono::nanoseconds duration) noexcept {
        _now += std::chrono::duration_cast<TimePoint::duration>(duration);
    }

  private:
    TimePoint _now;  ///< Starts at the clock's epoch.
};
}  // namespace rtype::network::tests
