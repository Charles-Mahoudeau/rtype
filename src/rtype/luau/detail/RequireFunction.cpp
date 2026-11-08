/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RequireFunction
*/

#include "RequireFunction.hpp"

#include <lua.h>
#include <lualib.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "rtype/luau/ModuleLoader.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Result.hpp"

namespace rtype::luau::detail {
namespace {
constexpr int kLoaderUpvalue = 1;
constexpr int kKeyUpvalue = 2;
constexpr const char* kLoadersSlot = "rtype.luau.loaders";  ///< Registry table: token to loader, and `next`.
constexpr int kWhereLevel = 1;                              ///< The function that called `require`.

/// @brief Reads the string at a stack index.
std::string toString(lua_State* state, const int index) {
    std::size_t length = 0;
    const char* text = lua_tolstring(state, index, &length);
    return text == nullptr ? std::string{} : std::string{text, length};
}

/// @brief Finds the loader behind the token of the running function, nullptr if it is gone.
ModuleLoader* findLoader(lua_State* state) {
    const double token = lua_tonumber(state, lua_upvalueindex(kLoaderUpvalue));
    lua_getfield(state, LUA_REGISTRYINDEX, kLoadersSlot);
    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        return nullptr;
    }
    lua_pushnumber(state, token);
    lua_rawget(state, -2);
    auto* loader = static_cast<ModuleLoader*>(lua_tolightuserdata(state, -1));
    lua_pop(state, 2);
    return loader;
}
}  // namespace

int requireFunction(lua_State* state) {
    // Everything that owns memory lives in this scope: lua_error never runs with live C++ objects.
    {
        std::string where;
        std::string message;
        bool failed = true;
        luaL_where(state, kWhereLevel);
        where = toString(state, -1);
        lua_pop(state, 1);

        if (lua_type(state, 1) != LUA_TSTRING) {
            message = std::format("{}require expects a string path, got {}", where, luaL_typename(state, 1));
        } else if (ModuleLoader* loader = findLoader(state); loader == nullptr) {
            message = std::format("{}require: the module loader no longer exists", where);
        } else {
            const std::string key = toString(state, lua_upvalueindex(kKeyUpvalue));
            const std::string path = toString(state, 1);
            Result<Ref> result = loader->requireFrom(key, path);
            if (result) {
                result->push(state);
                failed = false;
            } else {
                message = std::format("{}require(\"{}\"): {}", where, path, result.error().message());
            }
        }
        if (!failed) {
            return 1;
        }
        lua_pushlstring(state, message.data(), message.size());
    }
    lua_error(state);
}

std::uint64_t attachLoader(lua_State* state, ModuleLoader* loader) {
    lua_getfield(state, LUA_REGISTRYINDEX, kLoadersSlot);
    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        lua_newtable(state);
        lua_pushvalue(state, -1);
        lua_setfield(state, LUA_REGISTRYINDEX, kLoadersSlot);
    }
    // `next` counts the tokens given: one is never given twice, unlike the address of a loader.
    lua_getfield(state, -1, "next");
    const auto token = static_cast<std::uint64_t>(lua_tonumber(state, -1)) + 1;
    lua_pop(state, 1);
    lua_pushnumber(state, static_cast<double>(token));
    lua_setfield(state, -2, "next");
    lua_pushnumber(state, static_cast<double>(token));
    lua_pushlightuserdata(state, loader);
    lua_rawset(state, -3);
    lua_pop(state, 1);
    return token;
}

void detachLoader(lua_State* state, const std::uint64_t token) noexcept {
    lua_getfield(state, LUA_REGISTRYINDEX, kLoadersSlot);
    if (lua_istable(state, -1)) {
        lua_pushnumber(state, static_cast<double>(token));
        lua_pushnil(state);
        lua_rawset(state, -3);
    }
    lua_pop(state, 1);
}

void pushRequire(lua_State* state, const std::uint64_t token, const std::string_view key) {
    lua_pushnumber(state, static_cast<double>(token));
    lua_pushlstring(state, key.data(), key.size());
    lua_pushcclosurek(state, requireFunction, "require", 2, nullptr);
}
}  // namespace rtype::luau::detail
