/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LinkSimulator
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"

namespace rtype::network::simulation {

/// @brief The longest latency or jitter the simulation accepts, in milliseconds.
inline constexpr std::uint16_t kMaxDelayMs = 5000;

/// @brief How a bad link is imitated: what each datagram, in each direction, suffers.
struct SimulationSettings {
    std::uint16_t latencyMs{0};  ///< Delay added to every datagram.
    std::uint16_t jitterMs{0};   ///< Extra random delay, from 0 to this value; it can reorder datagrams.
    float lossPercent{0.0F};     ///< Chance, from 0 to 100, that a datagram is dropped.

    /// @return Whether the settings change anything.
    [[nodiscard]] bool isActive() const noexcept { return latencyMs != 0 || jitterMs != 0 || lossPercent > 0.0F; }

    /// @return The settings with every value brought into its range: delays up to `kMaxDelayMs`, loss from 0 to 100
    /// (a number that is not a number counts as 0).
    [[nodiscard]] RTYPE_NETWORK_API SimulationSettings clamped() const noexcept;

    friend bool operator==(const SimulationSettings& left, const SimulationSettings& right) noexcept = default;
};

/// @brief A datagram held by the simulation.
struct SimulatedDatagram {
    io::Address peer;  ///< The sender of a received datagram, the destination of a sent one.
    Bytes bytes;       ///< Its content.
};

/// @brief One direction of a bad link: drops datagrams and holds the others back.
///
/// @details It knows no socket and no clock: the caller gives the datagrams and the time, and the random numbers come
/// from a `RandomSource` a test can fix. Off (the default settings) it changes nothing; the caller can skip it while
/// isIdle(). Datagrams with the same due time leave in the order they came. Not thread-safe.
class RTYPE_NETWORK_API LinkSimulator {
  public:
    /// @param random Where the drops and the jitter come from.
    explicit LinkSimulator(RandomSource random);

    /// @brief Replaces the settings. Datagrams already held keep their due time.
    /// @param settings The new settings, clamped.
    /// @return The settings in force.
    SimulationSettings configure(const SimulationSettings& settings);

    /// @return The settings in force.
    [[nodiscard]] const SimulationSettings& settings() const noexcept;

    /// @return Whether the simulation is off and holds nothing: the datagrams can bypass it.
    [[nodiscard]] bool isIdle() const noexcept;

    /// @brief Drops the datagram or holds it until its due time.
    /// @param datagram The datagram.
    /// @param now The current time.
    void submit(SimulatedDatagram datagram, TimePoint now);

    /// @brief Gives the datagrams due at @p now, in due order, and forgets them.
    [[nodiscard]] std::vector<SimulatedDatagram> takeReady(TimePoint now);

    /// @brief Gives every datagram held, due or not, in due order, and forgets them.
    [[nodiscard]] std::vector<SimulatedDatagram> takeAll();

    /// @return How many datagrams are held.
    [[nodiscard]] std::size_t pendingCount() const noexcept;

    /// @return How many datagrams the loss dropped.
    [[nodiscard]] std::uint64_t droppedCount() const noexcept;

  private:
    RandomSource _random;                                  ///< Drops and jitter.
    SimulationSettings _settings;                          ///< In force.
    std::multimap<TimePoint, SimulatedDatagram> _pending;  ///< By due time; equal keys keep their insertion order.
    std::uint64_t _dropped{0};                             ///< Dropped by the loss.
};

}  // namespace rtype::network::simulation
