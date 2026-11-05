/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Userdata
*/

#include "Userdata.hpp"

#include <lua.h>

#include <format>
#include <memory>
#include <typeindex>
#include <utility>

#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "detail/UsertypeData.hpp"
#include "detail/UsertypeRegistry.hpp"

namespace rtype::luau {
Result<Userdata> detail::readUserdata(lua_State* state, const int stackIndex) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (const int type = lua_type(state, stackIndex); type != LUA_TUSERDATA) {
        return Failure{ErrorKind::kTypeMismatch,
                       std::format("expected a userdata, got '{}'", lua_typename(state, type))};
    }
    return Userdata{Ref{state, stackIndex}};
}

bool Userdata::isValid() const noexcept { return _ref && _ref->isValid(); }

int Userdata::tag() const {
    if (!isValid()) {
        throw exceptions::InvalidRef{};
    }
    lua_State* const state{_ref->state()};
    _ref->push(state);
    const int tag{lua_userdatatag(state, -1)};
    lua_pop(state, 1);
    return tag;
}

Result<void> detail::pushUserdata(lua_State* state, const Userdata& userdata) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (!userdata._ref) {
        throw exceptions::InvalidRef{};
    }
    if (lua_mainthread(state) != userdata._ref->state()) {
        return Failure{ErrorKind::kRuntime, "the userdata belongs to another Lua state"};
    }
    userdata._ref->push(state);
    return {};
}

Userdata::Userdata(Ref ref) : _ref{std::make_shared<Ref>(std::move(ref))} {}

void* Userdata::pointerOf(const std::type_index type) const {
    if (!isValid()) {
        throw exceptions::InvalidRef{};
    }
    lua_State* const state{_ref->state()};
    const detail::UsertypeRegistry* registry{detail::UsertypeRegistry::of(state)};
    const detail::UsertypeData* usertype{registry != nullptr ? registry->find(type) : nullptr};
    if (usertype == nullptr) {
        return nullptr;
    }
    _ref->push(state);
    void* object{lua_touserdatatagged(state, -1, usertype->tag())};
    lua_pop(state, 1);
    return object;
}
}  // namespace rtype::luau
