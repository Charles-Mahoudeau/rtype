/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SimulatedLink
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

#include "rtype/network/Types.hpp"

namespace rtype::network::tests {
/// @brief The faults a `SimulatedLink` applies to the datagrams sent through it.
struct LinkConditions {
    double loss{0.0};       ///< Probability that a datagram is dropped.
    double duplicate{0.0};  ///< Probability that a datagram is delivered twice.
    double reorder{0.0};    ///< Probability that a datagram is held back so that later ones overtake it.
    std::chrono::milliseconds delay{0};
    std::chrono::milliseconds jitter{0};
};

/// @brief What a `SimulatedLink` did since its creation.
struct LinkStats {
    std::uint64_t sent{0};        ///< Datagrams given to `send`.
    std::uint64_t dropped{0};     ///< Datagrams lost.
    std::uint64_t duplicated{0};  ///< Extra copies scheduled.
    std::uint64_t reordered{0};   ///< Datagrams held back.
    std::uint64_t delivered{0};   ///< Datagrams returned by `receive`.
};

/// @brief A test-only network between the endpoints 0 and 1 that loses, duplicates, reorders and delays opaque
/// datagrams, deterministically from a seed.
class SimulatedLink {
  public:
    SimulatedLink(LinkConditions conditions, std::uint32_t seed);
    ~SimulatedLink() = default;
    SimulatedLink(const SimulatedLink&) = default;
    SimulatedLink& operator=(const SimulatedLink&) = default;
    SimulatedLink(SimulatedLink&&) noexcept = default;
    SimulatedLink& operator=(SimulatedLink&&) noexcept = default;

    /// @return The conditions applied to the datagrams sent from now on.
    [[nodiscard]] const LinkConditions& conditions() const noexcept;

    /// @return What the link did so far.
    [[nodiscard]] LinkStats stats() const noexcept;

    /// @brief Changes the conditions applied to the datagrams sent from now on; those in flight keep their fate.
    void setConditions(LinkConditions conditions) noexcept;

    /// @brief Hands `datagram` to the link for the endpoint `to` (0 or 1) and decides its fate.
    void send(int to, Bytes datagram, TimePoint now);

    /// @brief Removes and returns the datagrams for the endpoint `at` whose arrival time is at or before `now`,
    /// ordered by arrival time then by send order.
    [[nodiscard]] std::vector<Bytes> receive(int at, TimePoint now);

  private:
    struct InFlight {
        int to;               ///< Endpoint 0 or 1.
        TimePoint arrival;    ///< When `receive` may return it.
        std::uint64_t order;  ///< Send order, to break ties between equal arrival times.
        Bytes bytes;
    };

    /// @return A number in [0, 1) from `_random`, converted by hand: the standard distributions may give other
    /// numbers on another standard library, this gives the same everywhere.
    [[nodiscard]] double roll();

    /// @return A random duration in [0, jitter].
    [[nodiscard]] TimePoint::duration randomJitter();

    LinkConditions _conditions;       ///< Applied to the next datagrams.
    std::mt19937 _random;             ///< Seeded by the test; its output is fixed by the standard.
    std::vector<InFlight> _inFlight;  ///< Datagrams sent but not received yet.
    std::uint64_t _nextOrder{0};      ///< Send order of the next datagram.
    LinkStats _stats;                 ///< Counters.
};
}  // namespace rtype::network::tests
