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
#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "FunctionRef.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "ValuePack.hpp"

namespace rtype::luau {
Function Function::create(lua_State* state, std::string name) {
    Function func{state, BindingType::kNative, std::move(name)};
    lua_pushlightuserdata(state, &func);
    lua_pushcclosure(state, &Function::handle, func.cStrName(), 1);
    func.setRef(Ref::pop(state));
    return func;
}

Function Function::fromRef(Ref ref, std::string name) {
    lua_State* const state{ref.state()};
    return Function{state, BindingType::kScript, FunctionRef{std::move(ref)}, std::move(name)};
}

Function Function::fromRef(const FunctionRef& ref, std::string name) {
    return Function{ref.ref().state(), BindingType::kScript, ref, std::move(name)};
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

const FunctionRef& Function::ref() const noexcept { return _ref; }

void Function::setHandler(Handler handler) { _handler = std::move(handler); }

Result<ValuePack> Function::call(const ValuePack& args) const {
    if (_type == BindingType::kNative) {
        std::cerr << "warning: you are calling a native function from a script binding" << std::endl;
    }
    const int top{lua_gettop(_state)};
    _ref.ref().push(_state);
    auto argCount = args.push(_state);
    if (!argCount) {
        // Drop the function, to leave the stack as it was.
        lua_settop(_state, top);
        return forwardError<ValuePack>(std::move(argCount));
    }
    if (lua_pcall(_state, static_cast<int>(*argCount), LUA_MULTRET, 0) != LUA_OK) {
        // The error value is not always a string (e.g. `error({})`), in which case lua_tostring returns null.
        const char* message{lua_tostring(_state, -1)};
        auto error = message != nullptr
                         ? std::string{message}
                         : std::format("error object is a {} value", lua_typename(_state, lua_type(_state, -1)));
        lua_pop(_state, 1);
        return Failure{ErrorKind::kRuntime, std::move(error)};
    }
    // With LUA_MULTRET, every value above the saved top is a returned value.
    const auto resultCount{static_cast<std::size_t>(lua_gettop(_state) - top)};
    auto results = ValuePack::pop(_state, resultCount);
    if (!results) {
        // Drop the returned values left by the failed read, to leave the stack as it was.
        lua_settop(_state, top);
    }
    return results;
}

Function::Function(lua_State* state, const BindingType type, FunctionRef ref, std::string name)
    : _state{state}, _type{type}, _ref{std::move(ref)}, _name{std::move(name)} {}

Function::Function(lua_State* state, const BindingType type, std::string name)
    : _state{state}, _type{type}, _name{std::move(name)} {}

const char* Function::cStrName() const noexcept { return _name.c_str(); }

void Function::setName(std::string name) { _name = std::move(name); }

void Function::setRef(Ref ref) {
    if (_type == BindingType::kScript) {
        throw exceptions::InvalidOperation{"unable to bind a script function"};
    }
    _ref = FunctionRef{std::move(ref)};
}

void Function::rebindUpvalue(const Function* from, Function* to) const noexcept {
    if (!_ref.isValid()) {
        return;
    }
    _ref.ref().push(_state);
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
    std::string error{};
    std::size_t resultCount{};
    // Every C++ object of the call lives in the scope below, so it is destroyed before the Lua error is raised. Only
    // the exceptions of the module and the ones that do not derive from std::exception are caught: a Lua error
    // raised inside the handler is a C++ exception too, and must reach Luau untouched.
    if (!self->_handler) {
        error = std::format("native function '{}' has no handler", self->_name);
    } else {
        auto context = CallContext::fromStack(state);
        if (!context) {
            error = std::format("invalid argument to '{}' ({})", self->_name, context.error().message());
        } else {
            try {
                const ValuePack results{self->_handler(*context)};
                const auto pushed = results.push(state);
                if (pushed) {
                    resultCount = *pushed;
                } else {
                    error = std::format("native function '{}': {}", self->_name, pushed.error().message());
                }
            } catch (const exceptions::Base& e) {
                error = std::format("native function '{}': {}", self->_name, e.what());
            } catch (const std::exception&) {
                // Luau turns a std::exception into a Lua error itself, and its own errors are one too.
                throw;
            } catch (...) {
                // Luau only catches std::exception: anything else would escape lua_pcall.
                error = std::format("native function '{}' threw an unknown exception", self->_name);
            }
        }
    }
    if (!error.empty()) {
        lua_pushlstring(state, error.data(), error.size());
        // The string is copied onto the Lua stack, so it can be released before the error unwinds.
        std::string{}.swap(error);
        lua_error(state);
    }
    return static_cast<int>(resultCount);
}
}  // namespace rtype::luau
