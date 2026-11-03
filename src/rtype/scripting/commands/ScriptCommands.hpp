/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCommands
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/component/ComponentLayout.hpp"
#include "rtype/scripting/view/ViewScope.hpp"

namespace rtype::scripting::commands {
/// @brief What `ctx.commands` holds: the Commands of the system's run, and what builds the values it records.
///
/// @details Like a view, it only lives during the run that received it: the scope's generation moves on when `run`
/// returns, and every call on a handle of an older generation raises a Luau error.
struct RTYPE_SCRIPTING_API ScriptCommands {
    ecs::Commands* commands{nullptr};                     ///< The buffer of the run.
    const component::ComponentLayouts* layouts{nullptr};  ///< The components scripts declared, with their defaults.
    view::ViewScope* scope{nullptr};                      ///< The scope that invalidates the handle.
    std::uint64_t generation{0};                          ///< The scope's generation when the handle was created.
};

/// @brief Registers the `Commands` usertype of `ctx.commands` in a runtime.
///
/// @details Scripts call `commands:spawn({ ["game.Shield"] = { strength = 10 } })`, which returns the reserved
/// `Entity` (the table may be left out for an entity with no component), `commands:add(entity, "game.Shield", {...})`
/// and `commands:despawn(entity)`. A component's value is its defaults with the fields of the table written over them.
/// An unknown component (one no script declared), a field the component does not have, a value that does not fit or a
/// value that is not an `Entity` raises a Luau error before anything of the call is recorded. As in C++, a command on
/// an entity that is dead when the buffer is flushed does nothing.
/// @param runtime The runtime to register the usertype in; it must have the `Entity` usertype
/// (view::registerViewTypes()) and not the `Commands` one yet.
/// @return Nothing, or the `luau::ErrorKind::kLimitExceeded` error if @p runtime has no userdata tag left.
/// @throws luau::exceptions::InvalidOperation If `Entity` is missing or `Commands` is already registered.
[[nodiscard]] RTYPE_SCRIPTING_API luau::Result<> registerCommandsType(luau::Runtime& runtime);
}  // namespace rtype::scripting::commands
