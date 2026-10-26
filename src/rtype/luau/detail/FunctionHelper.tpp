/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionHelper
*/

#pragma once

#ifndef RTYPE_FUNCTIONHELPER_INCLUDING_TPP
#error "FunctionHelper.tpp is an implementation detail of FunctionHelper.hpp: include FunctionHelper.hpp instead"
#endif

#include <string>
#include <string_view>
#include <type_traits>

namespace rtype::luau::detail {
template <typename T>
// NOLINTNEXTLINE(*-use-internal-linkage)
constexpr std::string_view luaTypeName() noexcept {
    if constexpr (std::is_same_v<T, Value::Nil>) {
        return "nil";
    } else if constexpr (std::is_same_v<T, bool>) {
        return "boolean";
    } else if constexpr (std::is_same_v<T, double>) {
        return "number";
    } else if constexpr (std::is_same_v<T, Value::Vector>) {
        return "vector";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return "string";
    } else {
        static_assert(std::is_same_v<T, Table>, "luaTypeName: unknown Value alternative");
        return "table";
    }
}
}  // namespace rtype::luau::detail
