/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionCall
*/

#pragma once

#ifndef RTYPE_ECS_FUNCTION_CALL_INCLUDING_TPP
#error "FunctionCall.tpp is an implementation detail of FunctionCall.hpp: include FunctionCall.hpp instead"
#endif

#include <cstddef>
#include <cstring>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/exceptions/CallException.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace detail {
/// @brief False for every type, so that a static_assert using it only fires when its template is instantiated.
template <typename T>
inline constexpr bool kCallDependentFalse = false;

/// @brief Whether a parameter is the arguments of a call: `const Args&`, with `Args` declared as call data.
template <typename T>
struct IsCallArguments : std::false_type {};

/// @brief `const Args&` with `Args` call data: the arguments.
template <CallData T>
struct IsCallArguments<const T&> : std::true_type {};

/// @brief The last of `Types`, or void when there is none.
template <typename... Types>
struct LastOf {
    using Type = void;  ///< No type.
};

/// @brief The last of a non-empty list.
template <typename First, typename... Rest>
struct LastOf<First, Rest...> {
    using Type = std::tuple_element_t<sizeof...(Rest), std::tuple<First, Rest...>>;  ///< The last type.
};

/// @brief The first elements of a tuple type, chosen by `Indices`.
template <typename Tuple, typename Indices>
struct TakeFront;

/// @brief The elements at `Indices...` of `std::tuple<Types...>`.
template <typename... Types, std::size_t... Indices>
struct TakeFront<std::tuple<Types...>, std::index_sequence<Indices...>> {
    using Type = std::tuple<std::tuple_element_t<Indices, std::tuple<Types...>>...>;  ///< The chosen elements.
};

/// @brief What a call's function takes and returns, split into its system parameters, its arguments and its result.
template <typename Return, typename... Types>
struct CallShapeOf {
    /// @brief Whether the last parameter is the arguments.
    static constexpr bool kTakesArguments = IsCallArguments<typename LastOf<Types...>::Type>::value;

    /// @brief The arguments, or void.
    using Arguments = std::conditional_t<kTakesArguments, std::remove_cvref_t<typename LastOf<Types...>::Type>, void>;

    /// @brief The system parameters, as a std::tuple.
    using Params =
        TakeFront<std::tuple<Types...>, std::make_index_sequence<sizeof...(Types) - (kTakesArguments ? 1 : 0)>>::Type;

    /// @brief The result, or void.
    using Result = Return;
};

/// @brief The shape of a call's function pointer.
template <typename Pointer>
struct CallShape;

/// @brief A function pointer.
template <typename Return, typename... Types>
struct CallShape<Return (*)(Types...)> : CallShapeOf<Return, Types...> {};

/// @brief A noexcept function pointer.
template <typename Return, typename... Types>
struct CallShape<Return (*)(Types...) noexcept> : CallShapeOf<Return, Types...> {};

/// @brief Whether every element of a tuple type can be a call's system parameter: a system parameter that needs no
/// configuration.
template <typename ParamList>
struct AreCallParameters;

/// @brief Checks `Params...`.
template <typename... Params>
struct AreCallParameters<std::tuple<Params...>> {
    static constexpr bool kValue = ((SystemParameter<Params> && !ConfiguredParameter<Params>) && ...);  ///< The answer.
};

/// @return The description of the call data `T`, checked, or nothing for void.
/// @throws exceptions::CallException When the description breaks a rule of validateType().
template <typename T>
[[nodiscard]] std::optional<TypeInfo> describeCallType(const std::string& call) {
    if constexpr (std::is_void_v<T>) {
        return std::nullopt;
    } else {
        TypeInfo info = TypeTraits<T>::makeTypeInfo();
        if (const auto valid = validateType(info, "call data"); !valid) {
            throw exceptions::CallException{
                std::format("Engine call '{}': invalid call data: {}", call, valid.error().getMessage())};
        }
        info.layoutHash = computeLayoutHash(info);
        return info;
    }
}

template <typename Function, typename Arguments, typename Result, typename... Params>
FunctionCall<Function, Arguments, Result, std::tuple<Params...>>::FunctionCall(World& world, Function function)
    : _function{function}, _states{SystemParam<Params>::init(world)...} {}

template <typename Function, typename Arguments, typename Result, typename... Params>
std::unique_ptr<CallBody> FunctionCall<Function, Arguments, Result, std::tuple<Params...>>::bind(World& world) const {
    return std::make_unique<FunctionCall>(world, _function);
}

template <typename Function, typename Arguments, typename Result, typename... Params>
void FunctionCall<Function, Arguments, Result, std::tuple<Params...>>::addAccess(SystemAccess& access) const {
    addAccess(access, std::index_sequence_for<Params...>{});
}

template <typename Function, typename Arguments, typename Result, typename... Params>
void FunctionCall<Function, Arguments, Result, std::tuple<Params...>>::invoke(const SystemContext& context,
                                                                              std::span<const std::byte> arguments,
                                                                              std::span<std::byte> result) {
    call(context, arguments, result, std::index_sequence_for<Params...>{});
}

template <typename Function, typename Arguments, typename Result, typename... Params>
template <std::size_t... Indices>
void FunctionCall<Function, Arguments, Result, std::tuple<Params...>>::addAccess(
    SystemAccess& access, std::index_sequence<Indices...> /*indices*/) const {
    (SystemParam<Params>::addAccess(std::get<Indices>(_states), access), ...);
}

template <typename Function, typename Arguments, typename Result, typename... Params>
template <std::size_t... Indices>
void FunctionCall<Function, Arguments, Result, std::tuple<Params...>>::call(
    const SystemContext& context, std::span<const std::byte> arguments, std::span<std::byte> result,
    std::index_sequence<Indices...> /*indices*/) {
    // Plain data is copied in and out, never aliased, so any bytes of the right size do. Call data that owns memory (a
    // text, a list) is an object the caller built in its bytes (CallBuffer): it is read there, and the result replaces
    // the one built there.
    const auto run = [&] {
        if constexpr (std::is_void_v<Arguments>) {
            return _function(SystemParam<Params>::get(std::get<Indices>(_states), context)...);
        } else if constexpr (std::is_trivially_copyable_v<Arguments>) {
            Arguments value{};
            if constexpr (!std::is_empty_v<Arguments>) {
                std::memcpy(&value, arguments.data(), sizeof(Arguments));
            }
            return _function(SystemParam<Params>::get(std::get<Indices>(_states), context)..., value);
        } else {
            const Arguments& value = *storage::viewAs<Arguments>(arguments);
            return _function(SystemParam<Params>::get(std::get<Indices>(_states), context)..., value);
        }
    };
    if constexpr (std::is_void_v<Result>) {
        run();
    } else if constexpr (std::is_trivially_copyable_v<Result>) {
        const Result value = run();
        if constexpr (!std::is_empty_v<Result>) {
            std::memcpy(result.data(), &value, sizeof(Result));
        }
    } else {
        Result value = run();
        Result* target = storage::viewAs<Result>(result);
        std::destroy_at(target);
        std::construct_at(target, std::move(value));  // cannot throw: RTYPE_ECS_CALL_DATA checks it
    }
}
}  // namespace detail

template <typename Function>
CallInfo makeCallInfo(World& world, std::string name, Function function) {
    if constexpr (!requires { requires std::is_pointer_v<decltype(+function)>; }) {
        static_assert(detail::kCallDependentFalse<Function>,
                      "An engine call must be a function or a lambda without captures (and without `auto` "
                      "parameters): its data lives in resources, which its parameters name.");
        return {};
    } else {
        using Pointer = decltype(+function);
        using Shape = detail::CallShape<Pointer>;
        if constexpr (!detail::AreCallParameters<typename Shape::Params>::kValue) {
            static_assert(detail::kCallDependentFalse<Function>,
                          "An engine call's parameters must be system parameters that need no configuration "
                          "(Res<T>, ResMut<T>, NonSend<T>, NonSendMut<T>, Query<Terms...>, Commands&, EventReader<T>, "
                          "EventWriter<T>), then optionally its arguments, `const Args&` with Args declared with "
                          "RTYPE_ECS_CALL_DATA.");
            return {};
        } else if constexpr (!std::is_void_v<typename Shape::Result> && !CallData<typename Shape::Result>) {
            static_assert(detail::kCallDependentFalse<Function>,
                          "An engine call must return void or a type declared with RTYPE_ECS_CALL_DATA.");
            return {};
        } else {
            using Body = detail::FunctionCall<Pointer, typename Shape::Arguments, typename Shape::Result,
                                              typename Shape::Params>;
            CallInfo info{
                .name = name,
                .arguments = detail::describeCallType<typename Shape::Arguments>(name),
                .result = detail::describeCallType<typename Shape::Result>(name),
                .body = std::make_unique<Body>(world, +function),
            };
            info.body->addAccess(info.access);
            return info;
        }
    }
}
}  // namespace rtype::ecs
