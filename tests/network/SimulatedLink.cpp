/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SimulatedLink
*/

#include "SimulatedLink.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "rtype/network/Types.hpp"

namespace rtype::network::tests {
SimulatedLink::SimulatedLink(LinkConditions conditions, const std::uint32_t seed)
    : _conditions{std::move(conditions)}, _random{seed} {}

const LinkConditions& SimulatedLink::conditions() const noexcept { return _conditions; }

LinkStats SimulatedLink::stats() const noexcept { return _stats; }

void SimulatedLink::setConditions(LinkConditions conditions) noexcept { _conditions = std::move(conditions); }

double SimulatedLink::roll() {
    constexpr double kScale = 1.0 / 16777216.0;  // 2^-24
    return static_cast<double>(_random() >> 8U) * kScale;
}

TimePoint::duration SimulatedLink::randomJitter() {
    const auto jitter = std::chrono::duration_cast<TimePoint::duration>(_conditions.jitter);
    return TimePoint::duration{static_cast<TimePoint::rep>(roll() * static_cast<double>(jitter.count()))};
}

void SimulatedLink::send(const int to, Bytes datagram, const TimePoint now) {
    const auto delay = std::chrono::duration_cast<TimePoint::duration>(_conditions.delay);

    ++_stats.sent;
    if (roll() < _conditions.loss) {
        ++_stats.dropped;
        return;
    }
    TimePoint arrival = now + delay + randomJitter();
    if (roll() < _conditions.reorder) {
        arrival += std::max(delay, std::chrono::duration_cast<TimePoint::duration>(std::chrono::milliseconds{1}));
        ++_stats.reordered;
    }
    if (roll() < _conditions.duplicate) {
        _inFlight.push_back(
            {.to = to, .arrival = now + delay + randomJitter(), .order = _nextOrder++, .bytes = datagram});
        ++_stats.duplicated;
    }
    _inFlight.push_back({.to = to, .arrival = arrival, .order = _nextOrder++, .bytes = std::move(datagram)});
}

std::vector<Bytes> SimulatedLink::receive(const int at, const TimePoint now) {
    const auto due = [at, now](const InFlight& flight) { return flight.to == at && flight.arrival <= now; };
    const auto firstDue =
        std::ranges::stable_partition(_inFlight, [&due](const InFlight& flight) { return !due(flight); }).begin();
    std::ranges::sort(firstDue, _inFlight.end(), [](const InFlight& left, const InFlight& right) {
        return left.arrival != right.arrival ? left.arrival < right.arrival : left.order < right.order;
    });

    std::vector<Bytes> result;
    result.reserve(static_cast<std::size_t>(_inFlight.end() - firstDue));
    for (auto it = firstDue; it != _inFlight.end(); ++it) {
        result.push_back(std::move(it->bytes));
    }
    _inFlight.erase(firstDue, _inFlight.end());
    _stats.delivered += result.size();
    return result;
}
}  // namespace rtype::network::tests
