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
constexpr int kWhereLevel = 1;  ///< The function that called `require`.

/// @brief Reads the string at a stack index.
std::string toString(lua_State* state, const int index) {
    std::size_t length = 0;
    const char* text = lua_tolstring(state, index, &length);
    return text == nullptr ? std::string{} : std::string{text, length};
}

/// @brief Finds the loader behind the token of the running function, nullptr if it is gone.
ModuleLoader* findLoader(lua_State* state) {
    void* token = lua_tolightuserdata(state, lua_upvalueindex(kLoaderUpvalue));
    lua_pushlightuserdata(state, token);
    lua_rawget(state, LUA_REGISTRYINDEX);
    auto* loader = static_cast<ModuleLoader*>(lua_tolightuserdata(state, -1));
    lua_pop(state, 1);
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

void attachLoader(lua_State* state, ModuleLoader* loader) {
    lua_pushlightuserdata(state, loader);
    lua_pushlightuserdata(state, loader);
    lua_rawset(state, LUA_REGISTRYINDEX);
}

void detachLoader(lua_State* state, ModuleLoader* loader) noexcept {
    lua_pushlightuserdata(state, loader);
    lua_pushnil(state);
    lua_rawset(state, LUA_REGISTRYINDEX);
}

void pushRequire(lua_State* state, ModuleLoader* loader, const std::string_view key) {
    lua_pushlightuserdata(state, loader);
    lua_pushlstring(state, key.data(), key.size());
    lua_pushcclosurek(state, requireFunction, "require", 2, nullptr);
}
}  // namespace rtype::luau::detail
