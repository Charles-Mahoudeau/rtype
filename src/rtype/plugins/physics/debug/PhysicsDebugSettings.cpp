/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugSettings
*/

#include "plugins/physics/debug/PhysicsDebugSettings.hpp"

#include <format>
#include <string_view>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::physics::debug {
namespace {
/// @return The positive number at @p key of the `physicsDebug` section, or @p fallback.
/// @throws core::config::exceptions::SettingsException If it is not positive.
float readPositive(const core::config::Settings& settings, const std::string_view key, const float fallback) {
    const double value = settings.getNumber(key, fallback);
    if (value <= 0.0) {
        throw core::config::exceptions::SettingsException{
            std::format("Setting 'physicsDebug.{}' must be positive", key)};
    }
    return static_cast<float>(value);
}
}  // namespace

PhysicsDebugSettings PhysicsDebugSettings::fromSettings(const core::config::Settings& settings) {
    settings.checkKeys({"enabled", "drawAll", "outlineThickness", "velocityScale"}, "physicsDebug");
    PhysicsDebugSettings config;
    config.enabled = settings.getBool("enabled", config.enabled);
    config.drawAll = settings.getBool("drawAll", config.drawAll);
    config.outlineThickness = readPositive(settings, "outlineThickness", config.outlineThickness);
    config.velocityScale = readPositive(settings, "velocityScale", config.velocityScale);
    return config;
}
}  // namespace rtype::plugins::physics::debug
