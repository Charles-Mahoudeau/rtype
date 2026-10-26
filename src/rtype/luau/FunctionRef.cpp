/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionRef
*/

#include "FunctionRef.hpp"

#include <lua.h>

#include <format>
#include <memory>
#include <utility>

#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "Ref.hpp"
#include "Result.hpp"

namespace rtype::luau {
Result<FunctionRef> FunctionRef::fromStack(lua_State* state, const int stackIndex) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (const int type = lua_type(state, stackIndex); type != LUA_TFUNCTION) {
        return Failure{ErrorKind::kTypeMismatch,
                       std::format("expected a function, got '{}'", lua_typename(state, type))};
    }
    return FunctionRef{Ref{state, stackIndex}};
}

bool FunctionRef::isValid() const noexcept { return _ref && _ref->isValid(); }

const Ref& FunctionRef::ref() const {
    if (!_ref) {
        throw exceptions::InvalidRef{};
    }
    return *_ref;
}

Result<void> FunctionRef::push(lua_State* state) const {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (!_ref) {
        throw exceptions::InvalidRef{};
    }
    if (lua_mainthread(state) != _ref->state()) {
        return Failure{ErrorKind::kRuntime, "the function belongs to another Lua state"};
    }
    _ref->push(state);
    return {};
}

FunctionRef::FunctionRef(Ref ref) : _ref{std::make_shared<Ref>(std::move(ref))} {}
}  // namespace rtype::luau
