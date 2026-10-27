/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Function
*/

#pragma once

#ifndef RTYPE_FUNCTION_INCLUDING_TPP
#error "Function.tpp is an implementation detail of Function.hpp: include Function.hpp instead"
#endif

#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

#include "CallContext.hpp"
#include "Exceptions.hpp"
#include "Value.hpp"
#include "ValuePack.hpp"
#include "detail/FunctionHelper.hpp"

// NOLINTNEXTLINE(*-concat-nested-namespaces)
namespace rtype::luau {
namespace detail {
/// @brief Reads an argument of a call as the type of a handler parameter.
///
/// A parameter holding a `std::optional<U>` accepts a missing argument or `nil` as an empty optional.
/// @tparam T The type of the handler parameter.
/// @param args The arguments of the call.
/// @param i Index of the argument to read, from 0.
/// @return A reference to the value held by the argument, valid as long as @p args, or a copy of it in an optional
///         for an optional parameter.
/// @throws exceptions::RuntimeError If the argument does not hold the type of the parameter.
template <typename T>
// NOLINTNEXTLINE(*-use-internal-linkage)
typename ParamTraits<T>::ReadType readArg(const std::span<const Value> args, const std::size_t i) {
    using Param = ParamTraits<T>;
    using U = Param::ValueType;
    if constexpr (Param::kIsOptional) {
        if (i >= args.size() || args[i].is<Value::Nil>()) {  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            return {};
        }
    }
    assert(i < args.size());
    // NOLINTNEXTLINE(*-pro-bounds-avoid-unchecked-container-access)
    if (const auto* p = args[i].tryAs<U>()) {
        return *p;
    }
    // NOLINTNEXTLINE(*-pro-bounds-avoid-unchecked-container-access)
    throw exceptions::RuntimeError{
        std::format("argument {}: expected {}, got {}", i + 1, luaTypeName<U>(), luaTypeNameOf(args[i]))};
}

/// @brief Wraps a typed handler in a `Function::Handler` that checks and reads the arguments of a call.
///
/// The handler built raises an `exceptions::RuntimeError` if the argument count is outside the range the parameters
/// allow, or if an argument does not hold the type of its parameter, without running @p handler.
/// @tparam T Types of the parameters of the handler.
/// @param handler The typed handler to run.
/// @return The handler to give to `Function::setHandler`.
template <typename... T>
// NOLINTNEXTLINE(*-use-internal-linkage)
Function::Handler makeBoundHandler(std::function<ValuePack(T...)> handler) {
    using Traits = FunctionTraits<std::function<ValuePack(T...)>>;
    return [handler = std::move(handler)](const CallContext& context) -> ValuePack {
        const auto args = context.args();
        if (args.size() < Traits::kRequiredCount || args.size() > Traits::kArgCount) {
            if constexpr (Traits::kRequiredCount == Traits::kArgCount) {
                throw exceptions::RuntimeError{
                    std::format("expected {} arguments, got {}", Traits::kArgCount, args.size())};
            } else {
                throw exceptions::RuntimeError{std::format("expected {} to {} arguments, got {}",
                                                           Traits::kRequiredCount, Traits::kArgCount, args.size())};
            }
        }
        return [&]<std::size_t... I>(std::index_sequence<I...>) -> ValuePack {
            // A braced initializer list evaluates left to right, so the first bad argument is the one reported.
            std::tuple<typename ParamTraits<T>::ReadType...> values{readArg<T>(args, I)...};
            return std::apply(handler, std::move(values));
        }(std::index_sequence_for<T...>{});
    };
}
}  // namespace detail

template <typename F>
void Function::bind(F&& f) {
    static_assert(std::is_copy_constructible_v<std::decay_t<F>>,
                  "bind callables must be copy-constructible (std::function requirement): no move-only captures");
    auto function = std::function{std::forward<F>(f)};
    using Traits = detail::FunctionTraits<decltype(function)>;
    static_assert(std::is_same_v<typename Traits::ReturnType, ValuePack>, "bind handlers must return ValuePack");
    static_assert(Traits::kArgsByValueOrConstRef, "bind handler parameters must be by value or const&");
    static_assert(Traits::template kArgsInVariant<Value::Variant>,
                  "bind handler parameters must be one of the alternatives of Value::Variant, or a std::optional of "
                  "one (use double for numbers, not int or float; std::string for strings, not std::string_view)");
    static_assert(Traits::kOptionalsTrailing, "bind handler: no required parameter may follow a std::optional one");
    setHandler(detail::makeBoundHandler(std::move(function)));
}
}  // namespace rtype::luau
