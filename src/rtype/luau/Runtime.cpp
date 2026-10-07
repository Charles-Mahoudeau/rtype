/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <lua.h>
#include <lualib.h>
#include <utility>

#include "ErrorKind.hpp"
#include "Failure.hpp"
#include "Result.hpp"
#include "RuntimeConfig.hpp"

namespace rtype::luau {
void Runtime::StateDeleter::operator()(lua_State* state) const noexcept { lua_close(state); }

Runtime::Runtime(lua_State* state, RuntimeConfig config) noexcept : _config{std::move(config)}, _state{state} {}

Result<Runtime> Runtime::create(RuntimeConfig config) noexcept {
    lua_State* state = luaL_newstate();
    if (state == nullptr) {
        return Failure{ErrorKind::kUnknown, "unable to create lua state"};
    }
    return Runtime{state, std::move(config)};
}

Runtime::~Runtime() noexcept = default;

Runtime::Runtime(Runtime&& other) noexcept = default;

Runtime& Runtime::operator=(Runtime&& other) noexcept = default;
}  // namespace rtype::luau
