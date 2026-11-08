/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpClient
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/http/Export.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::http {

namespace detail {
class CurlClient;
}  // namespace detail

/// @brief The id of a request, from HttpClient::send(); 0 is no request.
using RequestId = std::uint32_t;

/// @brief The HttpResult::error of a request that found the queue full (HttpSettings::maxQueued): it was not sent.
inline constexpr std::string_view kQueueFull = "queue_full";

/// @brief One request to send.
struct HttpRequest {
    std::string method{"GET"};  ///< GET, POST, PUT, PATCH, DELETE or HEAD.
    std::string url;            ///< `https://host/path`, to an allowed host.
    std::string headers;        ///< One `Name: value` per line; `{{env:NAME}}` is replaced by a listed secret.
    std::string body;           ///< Sent as is; empty for none.
    double timeout{0.0};        ///< Seconds from send(), time queued included; 0 or too long: the settings' timeout.
    std::int32_t priority{0};   ///< Higher is sent first when requests wait for a slot; the same: in send order.
};

/// @brief How a request ended.
struct HttpResult {
    RequestId request{0};    ///< Its id.
    std::int32_t status{0};  ///< The HTTP status (200, 404...); 0 when no response came (network error, timeout).
    std::string body;        ///< The response body; empty on an error.
    std::string error;       ///< Why it failed before a response came; empty when a response came. "queue_full" when
                             ///< the queue was full (kQueueFull), "timeout" when it ran out of time while queued.
};

/// @brief The HTTP client of the HttpPlugin, as the ECS resource "rtype.HttpClient": requests are sent at once and
/// never wait; poll() advances them and gives the ones that ended.
///
/// @details It sends at most HttpSettings::maxConcurrent requests at a time and queues the others (at most
/// HttpSettings::maxQueued; the next ones end with kQueueFull, never silently dropped), highest priority first and
/// in send order among equals. A request's timeout counts from send(): one that runs out of time while queued is never
/// sent, and ends with a timeout error. It only
/// reaches the allowed hosts, over HTTPS unless HttpSettings::allowHttp, follows no redirect, and stops a response
/// longer than HttpSettings::maxResponseBytes. Nothing it does blocks: no thread, the transfers advance in poll().
class RTYPE_PLUGIN_HTTP_API HttpClient {
  public:
    explicit HttpClient(HttpSettings settings);
    ~HttpClient();
    HttpClient(const HttpClient& other) = delete;
    HttpClient& operator=(const HttpClient& other) = delete;
    HttpClient(HttpClient&& other) noexcept;
    HttpClient& operator=(HttpClient&& other) noexcept;

    /// @brief Starts @p request, or queues it when HttpSettings::maxConcurrent are in flight.
    /// @return Its id, which its HttpResult carries (kQueueFull in the next poll() if the queue was full).
    /// @throws exceptions::HttpException If its method, URL or headers are invalid, its host is not allowed, or a
    /// header names a secret the settings do not list or that is not set.
    RequestId send(const HttpRequest& request);

    /// @brief Stops @p request; it gives no result.
    /// @return False, doing nothing, if no request in flight or queued has this id.
    bool cancel(RequestId request);

    /// @brief Advances the transfers, without waiting, and starts queued requests as others end.
    /// @return The requests that ended since the last call, in the order they ended.
    [[nodiscard]] std::vector<HttpResult> poll();

    /// @return The requests in flight and queued.
    [[nodiscard]] std::size_t getPendingCount() const noexcept;

  private:
    std::unique_ptr<detail::CurlClient> _client;  ///< The curl transfers; private to the plugin.
};
RTYPE_ECS_RESOURCE(HttpClient, "rtype.HttpClient");

}  // namespace rtype::plugins::http
