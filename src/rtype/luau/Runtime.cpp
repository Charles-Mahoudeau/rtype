/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <lua.h>
#include <lualib.h>

#include <optional>

namespace rtype::luau {
void Runtime::StateDeleter::operator()(lua_State* state) const noexcept { lua_close(state); }

std::optional<Runtime> Runtime::create() noexcept {
    lua_State* state = luaL_newstate();
    if (state == nullptr) {
        return std::nullopt;
    }
    return Runtime{state};
}

Runtime::Runtime(lua_State* state) noexcept : _state{state} {}

Runtime::~Runtime() noexcept = default;

Runtime::Runtime(Runtime&& other) noexcept = default;

Runtime& Runtime::operator=(Runtime&& other) noexcept = default;
}  // namespace rtype::luau
