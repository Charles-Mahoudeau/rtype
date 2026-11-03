/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ViewTypes
*/

#pragma once

#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/scripting/Export.hpp"

namespace rtype::scripting::view {
/// @brief Registers what scripts use to reach ECS data in a runtime: the `Entity` usertype and the `ComponentView` one.
///
/// @details `Entity` is an opaque handle: `==` compares two of them, `tostring` shows `Entity(index:generation)`, and
/// `bits` gives a number to use as a table key (exact while the slot's generation stays under 2^21). A
/// `ComponentView` reads and writes a field with `view.name` and `view.name = value`; an unknown field, a value that
/// does not fit, a write through a read-only view or any access once the view's scope moved on raises a Luau error.
/// @param runtime The runtime to register the usertypes in; it must not have them yet.
/// @return Nothing, or the `luau::ErrorKind::kLimitExceeded` error if @p runtime has no userdata tag left.
/// @throws luau::exceptions::InvalidOperation If one of the types is already registered in @p runtime.
[[nodiscard]] RTYPE_SCRIPTING_API luau::Result<> registerViewTypes(luau::Runtime& runtime);
}  // namespace rtype::scripting::view
