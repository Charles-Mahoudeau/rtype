/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCallable
*/

#pragma once

namespace rtype::ecs {
/// @brief A callable that keeps state, given as a system: the one way a script runtime (the Luau bridge) passes the
/// function a script declared, through a lambda capturing a handle to it.
///
/// @details A plain system is a function or a lambda without captures: its data lives in components and resources,
/// where the scheduler sees it. ScriptCallable is the explicit exception, for callables whose state is only a
/// reference to code that lives elsewhere (a script function in its VM): never put game state in the captures. The
/// parameters are deduced from the call operator, as for a function, so they declare the system's access as usual
/// (DynamicQueries for the components a script names), and the callable must take explicit parameter types (no `auto`)
/// and return void. It is moved into the system: a move-only capture works.
/// @code
/// auto run = [script](rtype::ecs::DynamicQueries queries, rtype::ecs::Commands& commands) {
///     script.call(queries, commands);
/// };
/// app.addSystem(rtype::ecs::Phase::kFixedUpdate, "game.ShieldRegen", rtype::ecs::ScriptCallable{run},
///               rtype::ecs::QueryList{{rtype::ecs::QueryBuilder{}.write(shield)}});
/// @endcode
template <typename Callable>
struct ScriptCallable {
    Callable callable;  ///< Called at each run with the system's parameters.
};

/// @brief Deduces the callable's type: `ScriptCallable{[handle](...) {...}}`.
template <typename Callable>
ScriptCallable(Callable) -> ScriptCallable<Callable>;
}  // namespace rtype::ecs
