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
#include "detail/StateLife.hpp"  // IWYU pragma: keep
#include "detail/UsertypeRegistry.hpp"

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
    if (const auto* registry = detail::UsertypeRegistry::of(state); registry != nullptr) {
        _life = registry->life();
    }
}

Ref::~Ref() { release(); }

Ref::Ref(Ref&& other) noexcept
    : _state{other._state}, _ref{std::exchange(other._ref, kLuaNoRef)}, _life{std::move(other._life)} {}

Ref& Ref::operator=(Ref&& other) noexcept {
    if (this != &other) {
        release();
        _state = other._state;
        _ref = std::exchange(other._ref, kLuaNoRef);
        _life = std::move(other._life);
    }
    return *this;
}

lua_State* Ref::state() const noexcept { return _state; }

void Ref::push(lua_State* state) const {
    if (!isValid()) {
        throw exceptions::InvalidRef{};
    }
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    lua_getref(state, _ref);
}

bool Ref::isValid() const noexcept { return _state != nullptr && _ref != kLuaNoRef && isStateOpen(); }

bool Ref::isNil() const noexcept { return _ref == LUA_REFNIL; }

bool Ref::isStateOpen() const noexcept { return !_life || _life->isAlive(); }

void Ref::release() noexcept {
    // A closed state took its registry with it: there is nothing left to release.
    if (_state != nullptr && _ref != kLuaNoRef && isStateOpen()) {
        lua_unref(_state, _ref);
    }
}
}  // namespace rtype::luau
