/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Ref
*/

#include "Ref.hpp"

#include <lua.h>

#include <utility>

#include "Exceptions.hpp"

namespace rtype::luau {
static_assert(Ref::kLuaNoRef == LUA_NOREF);

Ref Ref::pop(lua_State* state) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    Ref ref{state, kLuaStackTop};
    lua_pop(state, 1);
    return ref;
}

Ref::Ref(lua_State* state, const int stackIndex) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    _ref = lua_ref(state, stackIndex);
    _state = lua_mainthread(state);
}

Ref::~Ref() {
    if (_state != nullptr && _ref != kLuaNoRef) {
        lua_unref(_state, _ref);
    }
}

Ref::Ref(Ref&& other) noexcept : _state{other._state}, _ref{std::exchange(other._ref, kLuaNoRef)} {}

Ref& Ref::operator=(Ref&& other) noexcept {
    if (this != &other) {
        if (_state != nullptr && _ref != kLuaNoRef) {
            lua_unref(_state, _ref);
        }
        _state = other._state;
        _ref = std::exchange(other._ref, kLuaNoRef);
    }
    return *this;
}

void Ref::push(lua_State* state) const {
    if (!isValid()) {
        throw exceptions::InvalidRef{};
    }
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    lua_getref(state, _ref);
}

bool Ref::isValid() const noexcept { return _state != nullptr && _ref != kLuaNoRef; }

bool Ref::isNil() const noexcept { return _ref == LUA_REFNIL; }
}  // namespace rtype::luau
