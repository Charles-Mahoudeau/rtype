/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineCall
*/

#pragma once

#include <cstddef>
#include <functional>
#include <span>

#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "rtype/scripting/Export.hpp"

namespace rtype::scripting::call {
/// @brief What actually calls an engine call: Calls::invoke() while scripts load, DynamicCalls::invoke() in a system.
using Invoker = std::function<void(std::span<const std::byte> arguments, std::span<std::byte> result)>;

/// @brief Calls an engine call with the fields a script gave, and gives its result back as Luau values.
///
/// @details The arguments start as their defaults (the call data's default member initializers), then the fields of
/// `fields` are written over them with the conversions of component fields (an `assetId` takes a path, a `vec2` a
/// table...). The result is nothing when the call returns nothing or a result without fields, the value of its field
/// when it has one, and a table of its fields otherwise.
/// @param runtime The runtime of the script, to build the values of the result.
/// @param info The call.
/// @param fields `nil`, nothing, or a table of argument fields.
/// @param invoke Calls the call with the bytes of the arguments and of the result.
/// @return The result's values.
/// @throws luau::exceptions::RuntimeError Naming the call, when `fields` is not a table, names a field the arguments
/// do not have or a value of the wrong type, gives fields to a call that takes none, or when the call throws.
[[nodiscard]] RTYPE_SCRIPTING_API luau::ValuePack callEngine(const luau::Runtime& runtime, const ecs::CallInfo& info,
                                                             const luau::Value* fields, const Invoker& invoke);
}  // namespace rtype::scripting::call
