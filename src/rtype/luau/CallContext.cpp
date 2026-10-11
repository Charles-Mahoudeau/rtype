/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContext
*/

#include "CallContext.hpp"

#include <cstddef>
#include <span>
#include <utility>

#include "Value.hpp"
#include "ValuePack.hpp"

namespace rtype::luau {
CallContext::CallContext(ValuePack args) noexcept : _args{std::move(args)} {}

std::span<const Value> CallContext::args() const noexcept { return _args.values(); }

std::size_t CallContext::argCount() const noexcept { return _args.size(); }
}  // namespace rtype::luau
