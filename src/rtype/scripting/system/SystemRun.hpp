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
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/luau/FunctionRef.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/view/ViewScope.hpp"

namespace rtype::scripting::system {
/// @brief A data term of a script system's query: what a script receives for it, after the entity.
struct DataTerm {
    const ecs::ComponentInfo* info{nullptr};  ///< The component; outlives the system (the registry never moves it).
    bool readOnly{false};                     ///< Whether the view refuses writes (a read term).
};

/// @brief What the body of a script system keeps between runs.
struct ScriptSystem {
    std::string name;                        ///< Name of the system, for the messages.
    luau::FunctionRef run;                   ///< The `run` function of the declaration.
    std::vector<DataTerm> terms;             ///< The data terms of its query, in the order the query yields them.
    std::shared_ptr<RuntimeHandle> runtime;  ///< The runtime of the host, null inside once it is destroyed.
    std::shared_ptr<view::ViewScope> scope;  ///< Invalidates the views of a run when it returns.
};

/// @brief Runs a script system once: calls its `run` with a `ctx`.
///
/// @details `ctx.query` is an iterator: `for entity, shield in ctx.query do` yields the entity and, for each data term
/// in the order of the query, a view of its component (read-only for a read term, `nil` for an optional component the
/// entity lacks). `ctx.delta` and `ctx.fixedDelta` are the durations of Time, and `ctx.commands` records structural
/// changes (see registerCommandsType()). The views die when `run` returns. A Luau
/// error in `run` is written to `std::cerr` with the name of the system and skips the rest of the run; nothing is
/// thrown.
/// @param system The system to run.
/// @param queries The system's queries, of which the first is iterated.
/// @param time The time of the frame.
/// @param commands The buffer `ctx.commands` records into, flushed by the scheduler between phases.
RTYPE_SCRIPTING_API void runScriptSystem(ScriptSystem& system, const ecs::DynamicQueries& queries,
                                         const ecs::Time& time, ecs::Commands& commands);
}  // namespace rtype::scripting::system
