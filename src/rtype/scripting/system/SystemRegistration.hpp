/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRegistration
*/

#pragma once

#include <memory>
#include <optional>

#include "RuntimeHandle.hpp"
#include "SystemDeclaration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::system {
/// @brief A script system made but not added to the schedule yet.
struct BuiltSystem {
    ecs::Phase phase;                ///< The phase it runs in.
    std::optional<ecs::Side> side;   ///< Where it runs, if the script said; not set on the system yet.
    ecs::Schedule::SystemSpec spec;  ///< The system, with the systems it runs after and before.
};

/// @brief Makes a script system as registerSystem() does, without adding it: the first half of registerSystem(), used
/// by hot reload to replace a system of the schedule.
/// @return The system, or the errors of registerSystem(), but a name already used: the schedule says that.
[[nodiscard]] RTYPE_SCRIPTING_API Result<BuiltSystem> makeSystem(ecs::App& app, SystemDeclaration declaration,
                                                                 std::shared_ptr<RuntimeHandle> runtime);

/// @brief Adds a script system to the App: looks the components of its query up, builds the QueryList and calls
/// App::addSystem with a ScriptCallable that runs the `run` function (see runScriptSystem()), then `after()` and
/// `before()` for its order.
///
/// @details The system's access set is the one of its query terms. An `after` or `before` naming a system that no
/// script or plugin added is only found when the schedule is built, since it may be added later.
/// @param app The App to add the system to.
/// @param declaration The system, as the script declared it.
/// @param runtime Gives the system its runtime, and tells it when the runtime is gone.
/// @return Nothing, or an ErrorKind::kInvalidDeclaration error (unknown component, terms that contradict each other)
/// or kRegistration (a system already has this name).
[[nodiscard]] RTYPE_SCRIPTING_API Result<void> registerSystem(ecs::App& app, SystemDeclaration declaration,
                                                              std::shared_ptr<RuntimeHandle> runtime);
}  // namespace rtype::scripting::system
