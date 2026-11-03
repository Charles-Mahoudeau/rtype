/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRegistration
*/

#pragma once

#include "SystemDeclaration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::system {
/// @brief Adds a script system to the App: looks the components of its query up, builds the QueryList and calls
/// App::addSystem with a ScriptCallable holding the `run` function, then `after()` and `before()` for its order.
///
/// @details The system's access set is the one of its query terms. An `after` or `before` naming a system that no
/// script or plugin added is only found when the schedule is built, since it may be added later.
/// @return Nothing, or an ErrorKind::kInvalidDeclaration error (unknown component, terms that contradict each other)
/// or kRegistration (a system already has this name).
[[nodiscard]] RTYPE_SCRIPTING_API Result<void> registerSystem(ecs::App& app, SystemDeclaration declaration);
}  // namespace rtype::scripting::system
