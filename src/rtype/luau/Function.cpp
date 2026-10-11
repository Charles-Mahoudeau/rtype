/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Function
*/

#include "Function.hpp"

#include <lua.h>

#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <utility>

#include "CallContext.hpp"
#include "CallResult.hpp"
#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "Ref.hpp"
#include "Result.hpp"

namespace rtype::luau {
Function Function::create(lua_State* state, std::string name) {
    Function func{state, BindingType::kNative, std::move(name)};
    lua_pushlightuserdata(state, &func);
    lua_pushcclosure(state, &Function::handle, func.cStrName(), 1);
    func.setRef(Ref::pop(state));
    func.ref().push(state);
    lua_setglobal(state, func.cStrName());
    return func;
}

Function Function::fromRef(Ref ref, std::string name) {
    return Function{ref.state(), BindingType::kScript, std::move(ref), std::move(name)};
}

Function::~Function() { rebindUpvalue(this, nullptr); }

Function::Function(Function&& other) noexcept
    : _state{other._state},
      _type{other._type},
      _ref{std::move(other._ref)},
      _name{std::move(other._name)},
      _handler{std::move(other._handler)} {
    if (_type == BindingType::kNative) {
        rebindUpvalue(&other, this);
    }
}

Function& Function::operator=(Function&& other) noexcept {
    if (this != &other) {
        if (_type == BindingType::kNative) {
            rebindUpvalue(this, nullptr);
        }
        _state = other._state;
        _type = other._type;
        _ref = std::move(other._ref);
        _name = std::move(other._name);
        _handler = std::move(other._handler);
        if (_type == BindingType::kNative) {
            rebindUpvalue(&other, this);
        }
    }
    return *this;
}

const Ref& Function::ref() const noexcept { return _ref; }

void Function::setHandler(Handler handler) { _handler = std::move(handler); }

Result<CallResult> Function::call(const CallContext& context) const {
    if (_type == BindingType::kNative) {
        std::cerr << "warning: you are calling a native function from a script binding" << std::endl;
    }
    const int top{lua_gettop(_state)};
    _ref.push(_state);
    std::size_t argCount{};
    try {
        argCount = context.pushArgs(_state);
    } catch (const std::exception& e) {
        // Drop the function and the arguments pushed before the failure, to leave the stack as it was.
        lua_settop(_state, top);
        return Failure{ErrorKind::kRuntime, e.what()};
    } catch (...) {
        lua_settop(_state, top);
        return Failure{ErrorKind::kRuntime, "unknown error"};
    }
    if (lua_pcall(_state, static_cast<int>(argCount), LUA_MULTRET, 0) != LUA_OK) {
        std::string error{lua_tostring(_state, -1)};
        lua_pop(_state, 1);
        return Failure{ErrorKind::kRuntime, std::move(error)};
    }
    // With LUA_MULTRET, every value above the saved top is a returned value.
    const auto resultCount{static_cast<std::size_t>(lua_gettop(_state) - top)};
    return CallResult::fromStack(_state, resultCount);
}

Function::Function(lua_State* state, const BindingType type, Ref ref, std::string name)
    : _state{state}, _type{type}, _ref{std::move(ref)}, _name{std::move(name)} {}

Function::Function(lua_State* state, const BindingType type, std::string name)
    : _state{state}, _type{type}, _name{std::move(name)} {}

const char* Function::cStrName() const noexcept { return _name.c_str(); }

void Function::setName(std::string name) { _name = std::move(name); }

void Function::setRef(Ref ref) {
    if (_type == BindingType::kScript) {
        throw exceptions::InvalidOperation{"unable to bind a script function"};
    }
    _ref = std::move(ref);
}

void Function::rebindUpvalue(const Function* from, Function* to) const noexcept {
    if (!_ref.isValid()) {
        return;
    }
    _ref.push(_state);
    if (lua_tocfunction(_state, -1) == &Function::handle && lua_getupvalue(_state, -1, 1) != nullptr) {
        const bool bound = lua_touserdata(_state, -1) == from;
        lua_pop(_state, 1);
        if (bound) {
            lua_pushlightuserdata(_state, to);
            lua_setupvalue(_state, -2, 1);
        }
    }
    lua_pop(_state, 1);
}

int Function::handle(lua_State* state) {
    auto* self = static_cast<Function*>(lua_touserdata(state, lua_upvalueindex(1)));
    if (self == nullptr) {
        lua_pushstring(state, "attempt to call a native function whose Function was destroyed");
        lua_error(state);
    }
    if (self->_type != BindingType::kNative) {
        throw exceptions::InvalidOperation{
            "somehow, a script function was bound to a native function, that's a massive fuck up, this is a critical "
            "bug"};
    }
    if (!self->_handler) {
        const std::string message{std::format("native function '{}' has no handler", self->_name)};
        lua_pushlstring(state, message.data(), message.size());
        lua_error(state);
    }
    self->_handler(CallContext{});
    return 0;
}
}  // namespace rtype::luau
