/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpSettings
*/

#include "plugins/http/HttpSettings.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::http {
namespace {
/// @return The whole number at @p key, from 1 to @p highest, or @p fallback.
/// @throws core::config::exceptions::SettingsException If it is not.
double readCount(const core::config::Settings& settings, const std::string_view key, const double fallback,
                 const double highest) {
    const double value = settings.getNumber(key, fallback);
    if (value < 1.0 || value > highest || std::floor(value) != value) {
        throw core::config::exceptions::SettingsException{
            std::format("Setting 'http.{}' must be a positive integer, at most {}", key, highest)};
    }
    return value;
}

/// @return Whether @p name can be an environment variable's name: letters, digits and underscores.
[[nodiscard]] bool isVariableName(const std::string_view name) {
    return !name.empty() && std::ranges::all_of(name, [](const char letter) {
        return (letter >= 'A' && letter <= 'Z') || (letter >= 'a' && letter <= 'z') ||
               (letter >= '0' && letter <= '9') || letter == '_';
    });
}
}  // namespace

HttpSettings HttpSettings::fromSettings(const core::config::Settings& settings) {
    settings.checkKeys(
        {"allowedHosts", "allowHttp", "maxConcurrent", "maxQueued", "timeout", "maxResponseBytes", "secrets"}, "http");
    // Bytes are counted in a double: exact up to 2^53, far more than a response holds.
    constexpr double kMaxBytes = 9007199254740992.0;
    HttpSettings config;
    config.allowedHosts = settings.getStringList("allowedHosts", {});
    for (std::string& host : config.allowedHosts) {
        if (host.empty() || host.find_first_of("/:@ ") != std::string::npos) {
            throw core::config::exceptions::SettingsException{std::format(
                "Setting 'http.allowedHosts': '{}' is not a host name (no scheme, port or path: 'api.example.com')",
                host)};
        }
        std::ranges::transform(host, host.begin(),
                               [](const unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    }
    config.allowHttp = settings.getBool("allowHttp", config.allowHttp);
    config.maxConcurrent = static_cast<std::uint32_t>(
        readCount(settings, "maxConcurrent", config.maxConcurrent, std::numeric_limits<std::uint32_t>::max()));
    // 0 is valid: nothing waits, a request over maxConcurrent ends with `queue_full`.
    const double queued = settings.getNumber("maxQueued", config.maxQueued);
    if (queued < 0.0 || queued > std::numeric_limits<std::uint32_t>::max() || std::floor(queued) != queued) {
        throw core::config::exceptions::SettingsException{"Setting 'http.maxQueued' must be an integer, 0 or more"};
    }
    config.maxQueued = static_cast<std::uint32_t>(queued);
    config.timeout = settings.getNumber("timeout", config.timeout);
    if (!(config.timeout > 0.0) || !std::isfinite(config.timeout)) {
        throw core::config::exceptions::SettingsException{
            "Setting 'http.timeout' must be a positive number of seconds"};
    }
    config.maxResponseBytes = static_cast<std::size_t>(
        readCount(settings, "maxResponseBytes", static_cast<double>(config.maxResponseBytes), kMaxBytes));
    config.secrets = settings.getStringList("secrets", {});
    for (const std::string& secret : config.secrets) {
        if (!isVariableName(secret)) {
            throw core::config::exceptions::SettingsException{std::format(
                "Setting 'http.secrets': '{}' is not an environment variable name (letters, digits and _)", secret)};
        }
    }
    return config;
}

}  // namespace rtype::plugins::http
