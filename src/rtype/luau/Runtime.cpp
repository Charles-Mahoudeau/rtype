/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <lua.h>
#include <lualib.h>

namespace rtype::luau {
void Runtime::StateDeleter::operator()(lua_State* state) const noexcept { lua_close(state); }

Runtime::Runtime() : _state{luaL_newstate()} {}

Runtime::~Runtime() = default;

Runtime::Runtime(Runtime&& other) noexcept = default;

Runtime& Runtime::operator=(Runtime&& other) noexcept = default;
}  // namespace rtype::luau
