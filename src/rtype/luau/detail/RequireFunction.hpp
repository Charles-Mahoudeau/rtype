/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RequireFunction
*/

#pragma once

#include <string_view>

struct lua_State;

namespace rtype::luau {
class ModuleLoader;
}  // namespace rtype::luau

namespace rtype::luau::detail {
/// @brief The `require` seen by a module: a C function with two upvalues, the loader's token and the module's key.
///
/// A Luau error is raised for an invalid argument and for every failure; the message reads
/// `<where>require("<path>"): <reason>`.
int requireFunction(lua_State* state);

/// @brief Records a loader in the registry, so the `require` functions can find it while it lives.
void attachLoader(lua_State* state, ModuleLoader* loader);

/// @brief Forgets a loader: its `require` functions raise an error from now on.
void detachLoader(lua_State* state, ModuleLoader* loader) noexcept;

/// @brief Pushes the `require` function of the module @p key.
void pushRequire(lua_State* state, ModuleLoader* loader, std::string_view key);
}  // namespace rtype::luau::detail
