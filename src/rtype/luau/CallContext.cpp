/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContext
*/

#include "CallContext.hpp"

#include <lua.h>

#include <cstddef>
#include <limits>
#include <utility>

#include "Exceptions.hpp"
#include "Value.hpp"

namespace rtype::luau {
void CallContext::addArg(Value value) { _args.push_back(std::move(value)); }

std::size_t CallContext::argCount() const { return _args.size(); }

std::size_t CallContext::pushArgs(lua_State* state) const {
    if (_args.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        lua_checkstack(state, static_cast<int>(_args.size())) == 0) {
        throw exceptions::StackOverflow{};
    }
    for (const auto& arg : _args) {
        arg.push(state);
    }
    return _args.size();
}
}  // namespace rtype::luau
