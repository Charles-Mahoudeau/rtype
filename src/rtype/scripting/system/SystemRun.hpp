/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRun
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "RuntimeHandle.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/DynamicCalls.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/system/DynamicEvents.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/DynamicResources.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/FunctionRef.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/view/ViewScope.hpp"

namespace rtype::scripting::system {
/// @brief A data term of a script system's query: what a script receives for it, after the entity.
struct DataTerm {
    const ecs::ComponentInfo* info{nullptr};  ///< The component; outlives the system (the registry never moves it).
    bool readOnly{false};                     ///< Whether the view refuses writes (a read term).
};

/// @brief A resource a script system declared: what `ctx.resources` gives for its name.
struct ResourceTerm {
    const ecs::TypeInfo* type{nullptr};  ///< The resource; outlives the system (the registry never moves it).
    bool write{false};                   ///< Whether the system may write it.
};

/// @brief An event type a script system declared: what `ctx.events` accepts for its name.
struct EventTerm {
    const ecs::TypeInfo* type{nullptr};  ///< The event; outlives the system (the registry never moves it).
    bool read{false};                    ///< Whether `ctx.events.read` accepts it.
    bool send{false};                    ///< Whether `ctx.events.send` accepts it.
};

/// @brief What the body of a script system keeps between runs.
struct ScriptSystem {
    std::string name;                         ///< Name of the system, for the messages.
    luau::FunctionRef run;                    ///< The `run` function of the declaration.
    std::vector<DataTerm> terms;              ///< The data terms of its query, in the order the query yields them.
    std::vector<ResourceTerm> resources;      ///< Its resources, in the order of its ResourceList.
    std::vector<EventTerm> events;            ///< Its event types, in the order of its EventList.
    std::vector<const ecs::CallInfo*> calls;  ///< Its engine calls, in the order of its CallList; owned by the world.
    bool clientSide{false};                   ///< Runs on clients only: its commands cannot add replicated components.
    const ecs::ComponentRegistry* components{nullptr};  ///< The world's components, for its commands.
    std::shared_ptr<RuntimeHandle> runtime;             ///< The runtime of the host, null inside once it is destroyed.
    std::shared_ptr<view::ViewScope> scope;             ///< Invalidates the views of a run when it returns.
};

/// @brief Runs a script system once: calls its `run` with a `ctx`.
///
/// @details `ctx.query` is an iterator: `for entity, shield in ctx.query do` yields the entity and, for each data term
/// in the order of the query, a view of its component (read-only for a read term, `nil` for an optional component the
/// entity lacks). `ctx.time` is a table of the values of Time (`delta`, `fixedDelta`, `elapsed`, `tick`, `alpha`),
/// `ctx.delta` and `ctx.fixedDelta` stay as shorthands of its first two, and `ctx.commands` records structural changes
/// (see registerCommandsType()). `ctx.resources["rtype.Input"]` is a view of a declared resource (read-only unless
/// declared `write`, `nil` while it has no value), and `ctx.events` has `send(name, fields)` and `read(name)` for the
/// declared events (see registerContextTypes()). `ctx.call(name, fields)` makes one of the engine calls the system
/// declared under `calls` (see call::callEngine()). The views die when `run` returns. A Luau error in `run` is written
/// to `std::cerr` with the name of the system and skips the rest of the run; nothing is thrown.
/// @param system The system to run.
/// @param queries The system's queries, of which the first is iterated.
/// @param resources The system's resources, in the order of `system.resources`.
/// @param events The system's event types, in the order of `system.events`.
/// @param calls The system's engine calls, in the order of `system.calls`.
/// @param time The time of the frame.
/// @param commands The buffer `ctx.commands` records into, flushed by the scheduler between phases.
RTYPE_SCRIPTING_API void runScriptSystem(ScriptSystem& system, const ecs::DynamicQueries& queries,
                                         ecs::DynamicResources& resources, const ecs::DynamicEvents& events,
                                         const ecs::DynamicCalls& calls, const ecs::Time& time,
                                         ecs::Commands& commands);
}  // namespace rtype::scripting::system
