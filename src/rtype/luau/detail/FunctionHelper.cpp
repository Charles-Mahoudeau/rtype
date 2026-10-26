/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionHelper
*/

#include "FunctionHelper.hpp"

#include <string>
#include <string_view>

#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"

namespace rtype::luau::detail {
std::string_view luaTypeNameOf(const Value& value) noexcept {
    // We cannot use std::visit here because it may throw.
    if (value.is<bool>()) {
        return luaTypeName<bool>();
    }
    if (value.is<double>()) {
        return luaTypeName<double>();
    }
    if (value.is<Value::Vector>()) {
        return luaTypeName<Value::Vector>();
    }
    if (value.is<std::string>()) {
        return luaTypeName<std::string>();
    }
    if (value.is<Table>()) {
        return luaTypeName<Table>();
    }
    return luaTypeName<Value::Nil>();
}
}  // namespace rtype::luau::detail
