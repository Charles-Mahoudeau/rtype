/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCommands
*/

#pragma once

#include <cstdint>
#include <memory>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
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
    const ecs::ComponentRegistry* registry{nullptr};      ///< The world's components, for the C++ ones; null for none.
    bool clientSide{false};  ///< The system runs on clients: it cannot add replicated or server-only components.
    std::weak_ptr<view::ViewScope> scope;  ///< The scope that invalidates the handle; expires with the system.
    std::uint64_t generation{0};           ///< The scope's generation when the handle was created.
};

/// @brief Registers the `Commands` usertype of `ctx.commands` in a runtime.
///
/// @details Scripts call `commands:spawn({ ["game.Shield"] = { strength = 10 } })`, which returns the reserved
/// `Entity` (the table may be left out for an entity with no component), `commands:add(entity, "game.Shield", {...})`
/// `commands:patch(entity, "game.Shield", { strength = 5 })`, which at the flush writes only the fields the table
/// names over the value the entity has then (a capsule just spawned keeps its other fields; skipped if it does not have
/// the component), `commands:setParent(child, parent)`, `commands:removeParent(child)` and `commands:despawn(entity)`,
/// which also despawns its children. A component is one a script declared or a C++ component of the world
/// (`rtype.Collider`, `rtype.Transform`, `rtype.TextRenderer`...), whose defaults are those of its C++ type: one that
/// owns memory (a `std::string` or `std::vector` field) is constructed, filled, copied into the command buffer and
/// destroyed. A component's value is its defaults with the fields of the table written over them. An unknown component,
/// a C++ component that owns memory but has no default constructor, a field the component does not have, a value that
/// does not fit or a value that is not an `Entity` raises a Luau error before anything of the call is recorded. As in
/// C++, a command on an entity that is dead when the buffer is flushed does nothing.
/// @param runtime The runtime to register the usertype in; it must have the `Entity` usertype
/// (view::registerViewTypes()) and not the `Commands` one yet.
/// @return Nothing, or the `luau::ErrorKind::kLimitExceeded` error if @p runtime has no userdata tag left.
/// @throws luau::exceptions::InvalidOperation If `Entity` is missing or `Commands` is already registered.
[[nodiscard]] RTYPE_SCRIPTING_API luau::Result<> registerCommandsType(luau::Runtime& runtime);
}  // namespace rtype::scripting::commands
