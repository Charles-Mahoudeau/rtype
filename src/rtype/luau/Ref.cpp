/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Ref
*/

#include "Ref.hpp"

#include <lua.h>

#include <utility>

namespace rtype::luau {
Ref::Ref(lua_State& state, const int stackIndex, const bool keepOnStack)
    : _state{state}, _ref{lua_ref(&_state.get(), stackIndex)} {
    if (!keepOnStack) {
        lua_pop(&_state.get(), 1);
    }
}

Ref::~Ref() {
    if (_ref != LUA_NOREF) {
        lua_unref(&_state.get(), _ref);
    }
}

Ref::Ref(Ref&& other) noexcept : _state{other._state}, _ref{std::exchange(other._ref, LUA_NOREF)} {}

Ref& Ref::operator=(Ref&& other) noexcept {
    if (this != &other) {
        lua_unref(&_state.get(), _ref);
        _state = other._state;
        _ref = std::exchange(other._ref, LUA_NOREF);
    }
    return *this;
}

void Ref::push() const { lua_getref(&_state.get(), _ref); }
}  // namespace rtype::luau
