/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpEvents
*/

#pragma once

#include <cstdint>
#include <string>

#include "rtype/ecs/event/EventTraits.hpp"

namespace rtype::plugins::http {

/// @brief Sent once a request of `rtype.http.request` ended, whatever its outcome; a cancelled request sends none.
/// It is sent in PreUpdate, so systems of Update read it in the same frame.
struct HttpResponse {
    std::uint32_t request{0};  ///< The id `rtype.http.request` gave.
    std::int32_t status{0};    ///< The HTTP status (200, 404...); 0 when no response came.
    bool ok{false};            ///< A response came with a status from 200 to 299.
    std::string body;          ///< The response body, of any length; empty on an error.
    std::string error;         ///< Why no response came (no network, a timeout, a body too long); empty otherwise.
};
RTYPE_ECS_EVENT(HttpResponse, "rtype.HttpResponse", request, status, ok, body, error);

}  // namespace rtype::plugins::http
