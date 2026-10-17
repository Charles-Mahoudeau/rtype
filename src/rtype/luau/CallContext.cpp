/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContext
*/

#include "CallContext.hpp"

#include <lua.h>

#include <cstddef>
#include <span>
#include <utility>

#include "Result.hpp"
#include "Value.hpp"
#include "ValuePack.hpp"

namespace rtype::luau {
Result<CallContext> CallContext::fromStack(lua_State* state) {
    auto args = ValuePack::fromStack(state, 1, static_cast<std::size_t>(lua_gettop(state)));

    if (!args) {
        return forwardError<CallContext>(std::move(args));
    }
    return CallContext{std::move(*args)};
}

CallContext::CallContext(ValuePack args) noexcept : _args{std::move(args)} {}

std::span<const Value> CallContext::args() const noexcept { return _args.values(); }

std::size_t CallContext::argCount() const noexcept { return _args.size(); }
}  // namespace rtype::luau
