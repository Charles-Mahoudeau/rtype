/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpSettings
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/http/Export.hpp"

namespace rtype::plugins::http {

/// @brief The `http` section of `project.luau`: what the HttpPlugin lets scripts reach, and its limits. Every key is
/// optional, but with no `allowedHosts` no request can be sent: a game names the servers it talks to.
struct HttpSettings {
    std::vector<std::string> allowedHosts;  ///< The hosts a request may go to, exactly ("api.example.com"), any case.
    bool allowHttp{false};                  ///< true: `http://` URLs too (a local server); false: `https://` only.
    std::uint32_t maxConcurrent{8};         ///< Requests in flight at once; the next ones wait their turn.
    double timeout{30.0};                   ///< Seconds a request may take, unless it asks for less.
    std::size_t maxResponseBytes{std::size_t{1024} * 1024};  ///< A longer response body fails with an error.
    std::vector<std::string> secrets;  ///< Environment variables a header may name as `{{env:NAME}}`; no others.

    /// @return The settings read from @p settings, the `http` section, each key left out keeping its default.
    /// @throws core::config::exceptions::SettingsException If a key is unknown, has the wrong type or an invalid value.
    [[nodiscard]] RTYPE_PLUGIN_HTTP_API static HttpSettings fromSettings(const core::config::Settings& settings);
};

}  // namespace rtype::plugins::http
