/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RawState
*/

#pragma once

#include "rtype/luau/Export.hpp"

struct lua_State;

namespace rtype::luau {
class Runtime;
}  // namespace rtype::luau

namespace rtype::luau::detail {
/// @brief Gives direct access to the main Lua state of a runtime.
///
/// @details An escape hatch for the internals of `luau` and its tests, which call the Luau C API: users of the
/// library make handles from the `Runtime`. The state stays owned by the runtime and must not be closed.
/// @return The main Lua state, valid as long as the runtime is alive.
[[nodiscard]] RTYPE_LUAU_API lua_State* rawState(const Runtime& runtime) noexcept;
}  // namespace rtype::luau::detail
