/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LinkSimulator
*/

#include "rtype/network/simulation/LinkSimulator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"

namespace rtype::network::simulation {
namespace {
/// Loss is compared in hundredths of a percent.
constexpr std::uint64_t kLossScale = 10000;
}  // namespace

SimulationSettings SimulationSettings::clamped() const noexcept {
    SimulationSettings result = *this;
    result.latencyMs = std::min(latencyMs, kMaxDelayMs);
    result.jitterMs = std::min(jitterMs, kMaxDelayMs);
    result.lossPercent = std::isnan(lossPercent) ? 0.0F : std::clamp(lossPercent, 0.0F, 100.0F);
    return result;
}

LinkSimulator::LinkSimulator(RandomSource random) : _random{std::move(random)} {}

SimulationSettings LinkSimulator::configure(const SimulationSettings& settings) {
    _settings = settings.clamped();
    return _settings;
}

const SimulationSettings& LinkSimulator::settings() const noexcept { return _settings; }

bool LinkSimulator::isIdle() const noexcept { return !_settings.isActive() && _pending.empty(); }

void LinkSimulator::submit(SimulatedDatagram datagram, const TimePoint now) {
    if (_settings.lossPercent > 0.0F) {
        const auto threshold = static_cast<std::uint64_t>(std::lround(_settings.lossPercent * 100.0F));
        if (_random() % kLossScale < threshold) {
            ++_dropped;
            return;
        }
    }
    std::uint64_t delayMs = _settings.latencyMs;
    if (_settings.jitterMs != 0) {
        delayMs += _random() % (static_cast<std::uint64_t>(_settings.jitterMs) + 1);
    }
    _pending.emplace(now + std::chrono::milliseconds{static_cast<std::int64_t>(delayMs)}, std::move(datagram));
}

std::vector<SimulatedDatagram> LinkSimulator::takeReady(const TimePoint now) {
    std::vector<SimulatedDatagram> ready;
    const auto end = _pending.upper_bound(now);
    for (auto it = _pending.begin(); it != end; ++it) {
        ready.push_back(std::move(it->second));
    }
    _pending.erase(_pending.begin(), end);
    return ready;
}

std::vector<SimulatedDatagram> LinkSimulator::takeAll() {
    std::vector<SimulatedDatagram> all;
    all.reserve(_pending.size());
    for (auto& [due, datagram] : _pending) {
        all.push_back(std::move(datagram));
    }
    _pending.clear();
    return all;
}

std::size_t LinkSimulator::pendingCount() const noexcept { return _pending.size(); }

std::uint64_t LinkSimulator::droppedCount() const noexcept { return _dropped; }

}  // namespace rtype::network::simulation
