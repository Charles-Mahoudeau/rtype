/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsSettings
*/

#include "plugins/physics/PhysicsSettings.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <string_view>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::physics {
namespace {
/// @return The whole number at @p key of the `physics` section, from 1 to @p highest, or @p fallback.
/// @throws core::config::exceptions::SettingsException If it is not.
double readCount(const core::config::Settings& settings, const std::string_view key, const double fallback,
                 const double highest) {
    const double value = settings.getNumber(key, fallback);
    if (value < 1.0 || value > highest || std::floor(value) != value) {
        throw core::config::exceptions::SettingsException{
            std::format("Setting 'physics.{}' must be a positive integer, at most {}", key, highest)};
    }
    return value;
}
}  // namespace

PhysicsSettings PhysicsSettings::fromSettings(const core::config::Settings& settings) {
    settings.checkKeys(
        {"gravity", "unitsPerMeter", "maxBodies", "maxBodyPairs", "maxContactConstraints", "tempAllocatorBytes"},
        "physics");
    constexpr auto kMaxCount = static_cast<double>(std::numeric_limits<std::uint32_t>::max());
    // Bytes are counted in a double: exact up to 2^53, far more than any allocator needs.
    constexpr double kMaxBytes = 9007199254740992.0;

    PhysicsSettings config;
    const std::vector<double> gravity = settings.getNumberList(
        "gravity", {static_cast<double>(config.gravity.x), static_cast<double>(config.gravity.y)});
    if (gravity.size() != 2) {
        throw core::config::exceptions::SettingsException{"Setting 'physics.gravity' must be { x, y }"};
    }
    config.gravity = {static_cast<float>(gravity.at(0)), static_cast<float>(gravity.at(1))};
    const double unitsPerMeter = settings.getNumber("unitsPerMeter", config.unitsPerMeter);
    if (unitsPerMeter <= 0.0) {
        throw core::config::exceptions::SettingsException{"Setting 'physics.unitsPerMeter' must be positive"};
    }
    config.unitsPerMeter = static_cast<float>(unitsPerMeter);
    config.maxBodies = static_cast<std::uint32_t>(readCount(settings, "maxBodies", config.maxBodies, kMaxCount));
    config.maxBodyPairs =
        static_cast<std::uint32_t>(readCount(settings, "maxBodyPairs", config.maxBodyPairs, kMaxCount));
    config.maxContactConstraints = static_cast<std::uint32_t>(
        readCount(settings, "maxContactConstraints", config.maxContactConstraints, kMaxCount));
    config.tempAllocatorBytes = static_cast<std::size_t>(
        readCount(settings, "tempAllocatorBytes", static_cast<double>(config.tempAllocatorBytes), kMaxBytes));
    return config;
}
}  // namespace rtype::plugins::physics
