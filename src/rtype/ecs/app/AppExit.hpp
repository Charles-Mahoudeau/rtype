/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AppExit
*/

#pragma once

#include "rtype/ecs/event/EventTraits.hpp"

namespace rtype::ecs {
/// @brief Asks the App to stop: App::run() returns `code` after the frame the event was sent in.
/// @details Any system may send it (the window plugin when the window closes, the server on a signal); when several
/// are sent, the first one read gives the code.
/// @code
/// void quit(rtype::ecs::EventWriter<rtype::ecs::AppExit> exit) {
///     exit.send(rtype::ecs::AppExit{.code = 0});
/// }
/// @endcode
struct AppExit {
    int code;  ///< What App::run() returns: 0 for success.
};
RTYPE_ECS_EVENT(AppExit, "rtype.AppExit", code);
}  // namespace rtype::ecs
