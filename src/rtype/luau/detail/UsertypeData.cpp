/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UsertypeData
*/

#include "UsertypeData.hpp"

#include <lua.h>

#include <cstddef>
#include <exception>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Failure.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using rtype::luau::Value;
using rtype::luau::detail::UsertypeData;

/// @brief Where `self` is in the arguments of a metamethod or a method.
constexpr int kSelfIndex = 1;
/// @brief Where the name of the member is in the arguments of `__index` and `__newindex`.
constexpr int kKeyIndex = 2;
/// @brief Where the written value is in the arguments of `__newindex`.
constexpr int kValueIndex = 3;
/// @brief Upvalue of the metamethods that holds the `UsertypeData`.
constexpr int kDataUpvalue = 1;
/// @brief Upvalue of `__index` and `__newindex` that holds the table of methods, and of a method that holds its entry.
constexpr int kSecondUpvalue = 2;

/// @brief Runs the body of a C closure, turning the exceptions of the module into a Luau error.
///
/// Every C++ object of the body is destroyed before the Lua error is raised, since the error unwinds the stack. Only
/// the exceptions of the module and the ones that do not derive from `std::exception` are caught here: a Lua error
/// raised inside the body is a C++ exception too, and must reach Luau untouched.
/// @tparam Body Callable without parameter that gives the number of results it pushed.
/// @tparam Label Callable without parameter that gives the prefix of the error message.
/// @return The number of results.
template <typename Body, typename Label>
int protect(lua_State* state, const Body& body, const Label& label) {
    std::string error{};
    int count{};
    try {
        count = body();
    } catch (const rtype::luau::exceptions::Base& e) {
        error = std::format("{}: {}", label(), e.what());
    } catch (const std::exception&) {
        throw;
    } catch (...) {
        error = std::format("{}: unknown exception", label());
    }
    if (!error.empty()) {
        lua_pushlstring(state, error.data(), error.size());
        // The string is copied onto the Lua stack, so it can be released before the error unwinds.
        std::string{}.swap(error);
        lua_error(state);
    }
    return count;
}

/// @brief Gets the `UsertypeData` a C closure belongs to.
UsertypeData& dataOf(lua_State* state) {
    return *static_cast<UsertypeData*>(lua_tolightuserdata(state, lua_upvalueindex(kDataUpvalue)));
}

/// @brief Gets the object of the userdata at a stack index, which must be of the type.
/// @throws exceptions::RuntimeError If the value is not a userdata of the type.
void* requireObject(lua_State* state, const UsertypeData& data, const int index) {
    if (void* object = data.objectAt(state, index)) {
        return object;
    }
    throw rtype::luau::exceptions::RuntimeError{
        std::format("self is not a {} (got {})", data.name(), lua_typename(state, lua_type(state, index)))};
}

/// @brief Reads the name of the member at a stack index; the view is valid as long as the string stays on the stack.
/// @throws exceptions::RuntimeError If the value is not a string.
std::string_view readKey(lua_State* state, const int index) {
    if (lua_type(state, index) != LUA_TSTRING) {
        throw rtype::luau::exceptions::RuntimeError{
            std::format("members are named by strings, got {}", lua_typename(state, lua_type(state, index)))};
    }
    std::size_t length{};
    const char* key{lua_tolstring(state, index, &length)};
    return {key, length};
}

/// @brief Reads the value at a stack index.
/// @throws exceptions::RuntimeError If the value has a type that cannot be read.
Value readValue(lua_State* state, const int index) {
    auto value = Value::fromStack(state, index);
    if (!value) {
        throw rtype::luau::exceptions::RuntimeError{std::string{value.error().message()}};
    }
    return std::move(*value);
}

/// @brief Pushes a value.
/// @throws exceptions::RuntimeError If the value cannot be pushed.
void pushValue(lua_State* state, const Value& value) {
    if (const auto pushed = value.push(state); !pushed) {
        throw rtype::luau::exceptions::RuntimeError{std::string{pushed.error().message()}};
    }
}

/// @brief Reads the arguments of a call, from a stack index up to the top.
/// @throws exceptions::RuntimeError If one of them has a type that cannot be read.
rtype::luau::ValuePack readArgs(lua_State* state, const int first) {
    const int count{lua_gettop(state) - first + 1};
    auto args = rtype::luau::ValuePack::fromStack(state, first, static_cast<std::size_t>(count < 0 ? 0 : count));
    if (!args) {
        throw rtype::luau::exceptions::RuntimeError{std::string{args.error().message()}};
    }
    return std::move(*args);
}
}  // namespace

namespace rtype::luau::detail {
UsertypeData::UsertypeData(lua_State* state, std::string name, const int tag, const std::size_t size,
                           const Destructor destructor)
    : _state{state}, _name{std::move(name)}, _tag{tag}, _size{size} {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    lua_setuserdatadtor(state, tag, destructor);
    lua_createtable(state, 0, 0);  // metatable
    lua_createtable(state, 0, 0);  // methods, referenced by `__methods` and by the upvalue of `__index`
    lua_pushvalue(state, -1);
    lua_setfield(state, -3, "__methods");
    lua_pushlightuserdata(state, this);
    lua_pushvalue(state, -2);
    lua_pushcclosure(state, &UsertypeData::index, "__index", 2);
    lua_setfield(state, -3, "__index");
    lua_pushlightuserdata(state, this);
    lua_pushvalue(state, -2);
    lua_pushcclosure(state, &UsertypeData::newIndex, "__newindex", 2);
    lua_setfield(state, -3, "__newindex");
    // `typeof` gives this name, and `getmetatable` gives the string, so scripts cannot reach the metatable.
    lua_pushlstring(state, _name.data(), _name.size());
    lua_setfield(state, -3, "__type");
    lua_pushstring(state, "locked");
    lua_setfield(state, -3, "__metatable");
    lua_pop(state, 1);
    lua_setuserdatametatable(state, tag);
}

const std::string& UsertypeData::name() const noexcept { return _name; }

int UsertypeData::tag() const noexcept { return _tag; }

Result<Userdata> UsertypeData::create(const Initializer initialize, void* source) const {
    const int top{lua_gettop(_state)};
    void* storage{nullptr};
    try {
        storage = allocate(_state);
    } catch (const std::exception&) {
        // Luau raises its allocation failures as C++ exceptions.
        lua_settop(_state, top);
        return Failure{ErrorKind::kOutOfMemory, "Lua out of memory"};
    }
    try {
        initialize(storage, source);
        seal(_state);
    } catch (const std::exception& e) {
        lua_settop(_state, top);
        return Failure{ErrorKind::kRuntime, e.what()};
    }
    auto userdata = Userdata::fromStack(_state);
    lua_settop(_state, top);
    return userdata;
}

void* UsertypeData::allocate(lua_State* state) const { return lua_newuserdatatagged(state, _size, 0); }

void UsertypeData::seal(lua_State* state) const {
    lua_setuserdatatag(state, -1, _tag);
    lua_getuserdatametatable(state, _tag);
    lua_setmetatable(state, -2);
}

void* UsertypeData::objectAt(lua_State* state, const int index) const {
    return lua_touserdatatagged(state, index, _tag);
}

void UsertypeData::addGetter(std::string name, Getter getter) {
    _getters.insert_or_assign(std::move(name), std::move(getter));
}

void UsertypeData::addSetter(std::string name, Setter setter) {
    _setters.insert_or_assign(std::move(name), std::move(setter));
}

void UsertypeData::addMethod(std::string name, Method method) {
    if (name.find('\0') != std::string::npos) {
        throw exceptions::InvalidOperation{"a method name cannot contain a NUL character"};
    }
    MethodEntry& entry = _methods.emplace_back(MethodEntry{.name = std::move(name), .handler = std::move(method)});
    pushMetatable();
    lua_getfield(_state, -1, "__methods");
    lua_pushlightuserdata(_state, this);
    lua_pushlightuserdata(_state, &entry);
    lua_pushcclosure(_state, &UsertypeData::callMethod, entry.name.c_str(), 2);
    lua_setfield(_state, -2, entry.name.c_str());
    lua_pop(_state, 2);
}

void UsertypeData::setConstructor(Constructor constructor) {
    if (!_constructor) {
        lua_getglobal(_state, _name.c_str());
        const bool taken = !lua_isnil(_state, -1);
        lua_pop(_state, 1);
        if (taken) {
            throw exceptions::InvalidOperation{
                std::format("cannot publish the constructor of '{}': a script global already has this name", _name)};
        }
    }
    _constructor = std::move(constructor);
    lua_createtable(_state, 0, 1);
    lua_pushlightuserdata(_state, this);
    lua_pushcclosure(_state, &UsertypeData::construct, "new", 1);
    lua_setfield(_state, -2, "new");
    lua_setglobal(_state, _name.c_str());
}

void UsertypeData::setIndexFallback(IndexFallback fallback) { _indexFallback = std::move(fallback); }

void UsertypeData::setNewIndexFallback(NewIndexFallback fallback) { _newIndexFallback = std::move(fallback); }

void UsertypeData::setEquality(Equality equality) {
    _equality = std::move(equality);
    pushMetatable();
    lua_pushlightuserdata(_state, this);
    lua_pushcclosure(_state, &UsertypeData::equal, "__eq", 1);
    lua_setfield(_state, -2, "__eq");
    lua_pop(_state, 1);
}

void UsertypeData::setStringifier(Stringifier stringifier) {
    _stringifier = std::move(stringifier);
    pushMetatable();
    lua_pushlightuserdata(_state, this);
    lua_pushcclosure(_state, &UsertypeData::stringify, "__tostring", 1);
    lua_setfield(_state, -2, "__tostring");
    lua_pop(_state, 1);
}

void UsertypeData::pushMetatable() const { lua_getuserdatametatable(_state, _tag); }

int UsertypeData::index(lua_State* state) {
    UsertypeData& data{dataOf(state)};
    std::string_view member{};
    return protect(
        state,
        [&] -> int {
            void* object{requireObject(state, data, kSelfIndex)};
            member = readKey(state, kKeyIndex);
            if (const auto getter = data._getters.find(member); getter != data._getters.end()) {
                pushValue(state, getter->second(object));
                return 1;
            }
            lua_pushvalue(state, kKeyIndex);
            lua_rawget(state, lua_upvalueindex(kSecondUpvalue));
            if (!lua_isnil(state, -1)) {
                return 1;
            }
            lua_pop(state, 1);
            if (data._indexFallback) {
                pushValue(state, data._indexFallback(object, member));
                return 1;
            }
            throw exceptions::RuntimeError{std::format("unknown member '{}'", member)};
        },
        [&] { return member.empty() ? data._name : std::format("{}.{}", data._name, member); });
}

int UsertypeData::newIndex(lua_State* state) {
    UsertypeData& data{dataOf(state)};
    std::string_view member{};
    return protect(
        state,
        [&] -> int {
            void* object{requireObject(state, data, kSelfIndex)};
            member = readKey(state, kKeyIndex);
            if (const auto setter = data._setters.find(member); setter != data._setters.end()) {
                setter->second(object, readValue(state, kValueIndex));
                return 0;
            }
            if (data._newIndexFallback) {
                data._newIndexFallback(object, member, readValue(state, kValueIndex));
                return 0;
            }
            lua_pushvalue(state, kKeyIndex);
            lua_rawget(state, lua_upvalueindex(kSecondUpvalue));
            const bool exists{data._getters.contains(member) || !lua_isnil(state, -1)};
            lua_pop(state, 1);
            if (exists) {
                throw exceptions::RuntimeError{std::format("member '{}' is read-only", member)};
            }
            throw exceptions::RuntimeError{std::format("unknown member '{}'", member)};
        },
        [&] { return member.empty() ? data._name : std::format("{}.{}", data._name, member); });
}

int UsertypeData::callMethod(lua_State* state) {
    UsertypeData& data{dataOf(state)};
    const auto& entry = *static_cast<const MethodEntry*>(lua_tolightuserdata(state, lua_upvalueindex(kSecondUpvalue)));
    return protect(
        state,
        [&]() -> int {
            void* object{requireObject(state, data, kSelfIndex)};
            const ValuePack args{readArgs(state, kSelfIndex + 1)};
            const ValuePack results{entry.handler(object, args.values())};
            const auto pushed = results.push(state);
            if (!pushed) {
                throw exceptions::RuntimeError{std::string{pushed.error().message()}};
            }
            return static_cast<int>(*pushed);
        },
        [&] { return std::format("{}:{}", data._name, entry.name); });
}

int UsertypeData::construct(lua_State* state) {
    UsertypeData& data{dataOf(state)};
    return protect(
        state,
        [&] -> int {
            const ValuePack args{readArgs(state, 1)};
            void* storage{data.allocate(state)};
            data._constructor(storage, args.values());
            data.seal(state);
            return 1;
        },
        [&] { return std::format("{}.new", data._name); });
}

int UsertypeData::equal(lua_State* state) {
    UsertypeData& data{dataOf(state)};
    return protect(
        state,
        [&] -> int {
            const void* lhs{data.objectAt(state, kSelfIndex)};
            const void* rhs{data.objectAt(state, kKeyIndex)};
            lua_pushboolean(state, lhs != nullptr && rhs != nullptr && data._equality(lhs, rhs));
            return 1;
        },
        [&] { return std::format("{}.__eq", data._name); });
}

int UsertypeData::stringify(lua_State* state) {
    UsertypeData& data{dataOf(state)};
    return protect(
        state,
        [&] -> int {
            const std::string text{data._stringifier(requireObject(state, data, kSelfIndex))};
            lua_pushlstring(state, text.data(), text.size());
            return 1;
        },
        [&] { return std::format("{}.__tostring", data._name); });
}
}  // namespace rtype::luau::detail
