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
Result<FunctionRef> detail::readFunctionRef(lua_State* state, const int stackIndex) {
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

Result<void> detail::pushFunctionRef(lua_State* state, const FunctionRef& function) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (!function._ref) {
        throw exceptions::InvalidRef{};
    }
    if (!function._ref->isStateOpen()) {
        return Failure{ErrorKind::kRuntime, "the runtime of the function was destroyed"};
    }
    if (lua_mainthread(state) != function._ref->state()) {
        return Failure{ErrorKind::kRuntime, "the function belongs to another Lua state"};
    }
    function._ref->push(state);
    return {};
}

FunctionRef::FunctionRef(Ref ref) : _ref{std::make_shared<Ref>(std::move(ref))} {}
}  // namespace rtype::luau
