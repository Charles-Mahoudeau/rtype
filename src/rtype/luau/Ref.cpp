/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Ref
*/

#include "Ref.hpp"

#include <lua.h>

#include <stdexcept>
#include <utility>

namespace rtype::luau {
Ref Ref::pop(lua_State* state) {
    Ref ref{state, kLuaStackTop};
    lua_pop(state, 1);
    return ref;
}

Ref::Ref(lua_State* state, const int stackIndex) {
    if (state == nullptr) {
        throw std::runtime_error{"invalid Lua state"};
    }
    _state = lua_mainthread(state);
    _ref = lua_ref(_state, stackIndex);
}

Ref::~Ref() {
    if (_ref != kLuaNoRef) {
        lua_unref(_state, _ref);
    }
}

Ref::Ref(Ref&& other) noexcept : _state{other._state}, _ref{std::exchange(other._ref, kLuaNoRef)} {}

Ref& Ref::operator=(Ref&& other) noexcept {
    if (this != &other) {
        lua_unref(_state, _ref);
        _state = other._state;
        _ref = std::exchange(other._ref, kLuaNoRef);
    }
    return *this;
}

void Ref::push() const { lua_getref(_state, _ref); }
}  // namespace rtype::luau
