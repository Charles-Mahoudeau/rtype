/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UsertypeRegistry
*/

#include "UsertypeRegistry.hpp"

#include <lua.h>

#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <typeindex>
#include <utility>

#include "UsertypeData.hpp"
#include "luaconf.h"
#include "rtype/luau/Exceptions.hpp"

namespace rtype::luau::detail {
UsertypeRegistry* UsertypeRegistry::of(lua_State* state) noexcept {
    return static_cast<UsertypeRegistry*>(lua_callbacks(state)->userdata);
}

UsertypeRegistry::UsertypeRegistry(lua_State* state) : _state{state} {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    lua_callbacks(state)->userdata = this;
}

UsertypeData* UsertypeRegistry::find(const std::type_index type) const noexcept {
    const auto it = _types.find(type);
    return it == _types.end() ? nullptr : it->second.get();
}

UsertypeData& UsertypeRegistry::add(const std::type_index type, std::string name, const std::size_t size,
                                    const UsertypeData::Destructor destructor) {
    if (_types.contains(type)) {
        throw exceptions::InvalidOperation{std::format("usertype '{}' is already registered", name)};
    }
    if (_nextTag >= LUA_UTAG_LIMIT) {
        throw exceptions::InvalidOperation{std::format(
            "unable to register usertype '{}': a Lua state has {} usertypes at most", name, LUA_UTAG_LIMIT - 1)};
    }
    auto data = std::make_unique<UsertypeData>(_state, std::move(name), _nextTag, size, destructor);
    ++_nextTag;
    return *_types.emplace(type, std::move(data)).first->second;
}
}  // namespace rtype::luau::detail
