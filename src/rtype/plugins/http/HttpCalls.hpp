/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpCalls
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::http {

inline constexpr std::string_view kRequestCall = "rtype.http.request";  ///< HttpClient::send().
inline constexpr std::string_view kCancelCall = "rtype.http.cancel";    ///< HttpClient::cancel().

/// @brief The arguments of `rtype.http.request`: one request, answered later by an HttpResponse.
struct RequestArguments {
    ecs::String method{"GET"};  ///< GET, POST, PUT, PATCH, DELETE or HEAD, any case.
    std::string url;            ///< `https://host/path`, to a host of http.allowedHosts.
    std::string headers;        ///< One `Name: value` per line; `{{env:NAME}}` is a secret of http.secrets.
    std::string body;           ///< Sent as is (a JSON text, for instance); empty for none.
    float timeout{0.0F};        ///< Seconds; 0: http.timeout, which is also the most it can be.
};
RTYPE_ECS_CALL_DATA(RequestArguments, "rtype.http.RequestArguments", method, url, headers, body, timeout);

/// @brief The result of `rtype.http.request`.
struct RequestResult {
    std::uint32_t request{0};  ///< Its id, which its HttpResponse carries.
};
RTYPE_ECS_CALL_DATA(RequestResult, "rtype.http.RequestResult", request);

/// @brief The arguments of `rtype.http.cancel`.
struct CancelArguments {
    std::uint32_t request{0};  ///< An id `rtype.http.request` gave.
};
RTYPE_ECS_CALL_DATA(CancelArguments, "rtype.http.CancelArguments", request);

/// @brief The result of `rtype.http.cancel`.
struct CancelResult {
    bool cancelled{false};  ///< False when the request had already ended, or no request has this id.
};
RTYPE_ECS_CALL_DATA(CancelResult, "rtype.http.CancelResult", cancelled);

/// @brief Registers `rtype.http.request` and `rtype.http.cancel` on @p app. A request that cannot be sent (an invalid
/// URL or header, a host that is not allowed, an unknown secret) throws an HttpException naming why, which a script
/// gets as a Luau error. Also registers the forms (addHttpForms()). Called by the HttpPlugin.
void addHttpCalls(ecs::App& app);

/// @brief Registers the Luau helper `engine.http.request(ctx, { url, method, headers, body, timeout })` on @p
/// registry: it checks its table, writes the headers table as lines, and makes the call. Its library does it on every
/// side, and so does building the plugin, for a program without libraries.
void addHttpForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::http
