/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <lua.h>
#include <lualib.h>

#include <expected>
#include <source_location>

#include "Error.hpp"
#include "ErrorKind.hpp"
#include "Result.hpp"

namespace rtype::luau {
void Runtime::StateDeleter::operator()(lua_State* state) const noexcept { lua_close(state); }

Result<Runtime> Runtime::create() noexcept {
    lua_State* state = luaL_newstate();
    if (state == nullptr) {
        return std::unexpected<Error>{{
            ErrorKind::kUnknown,
            "unable to create lua state",
            std::source_location::current(),
        }};
    }
    return Runtime{state};
}

Runtime::Runtime(lua_State* state) noexcept : _state{state} {}

Runtime::~Runtime() noexcept = default;

Runtime::Runtime(Runtime&& other) noexcept = default;

Runtime& Runtime::operator=(Runtime&& other) noexcept = default;
}  // namespace rtype::luau
