/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Usertype
*/

#pragma once

#ifndef RTYPE_USERTYPE_INCLUDING_TPP
#error "Usertype.tpp is an implementation detail of Usertype.hpp: include Usertype.hpp instead"
#endif

#include <functional>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Function.hpp"
#include "Value.hpp"
#include "ValuePack.hpp"
#include "detail/FunctionHelper.hpp"

namespace rtype::luau {
namespace detail {
/// @brief Destroys the object held by a userdata of `T`; registered as the destructor of the tag of `T`.
template <typename T>
// NOLINTNEXTLINE(*-use-internal-linkage)
void destroyUserdata(lua_State* /*state*/, void* object) {
    static_cast<T*>(object)->~T();
}

/// @brief Builds a `Value` from the result of a handler.
template <typename R>
// NOLINTNEXTLINE(*-use-internal-linkage)
Value toValue(R&& result) {
    if constexpr (std::is_same_v<std::remove_cvref_t<R>, Value>) {
        return std::forward<R>(result);
    } else {
        return Value{std::forward<R>(result)};
    }
}

/// @brief Builds the values a method gives back to the script from its result.
template <typename R>
// NOLINTNEXTLINE(*-use-internal-linkage)
ValuePack toPack(R&& result) {
    if constexpr (std::is_same_v<std::remove_cvref_t<R>, ValuePack>) {
        return std::forward<R>(result);
    } else {
        ValuePack pack{};
        pack.add(toValue(std::forward<R>(result)));
        return pack;
    }
}

/// @brief Checks the parameters of a handler that follow the object, as `Function::bind` does.
/// @tparam R Return type of the handler.
/// @tparam Rest Parameters of the handler after the object.
template <typename R, typename... Rest>
inline constexpr bool kValidParams = [] {
    using Traits = FunctionTraits<std::function<R(Rest...)>>;
    static_assert(Traits::kArgsByValueOrConstRef, "usertype handler parameters must be by value or const&");
    static_assert(Traits::template kArgsInVariant<Value::Variant>,
                  "usertype handler parameters must be one of the alternatives of Value::Variant, or a std::optional "
                  "of one (use double for numbers, not int or float; std::string for strings, not std::string_view)");
    static_assert(Traits::kOptionalsTrailing, "usertype handler: no required parameter may follow a std::optional one");
    return true;
}();

/// @brief Checks that the first parameter of a handler is the object, as a reference.
template <typename T, typename Self>
inline constexpr bool kIsSelf = std::is_lvalue_reference_v<Self> && std::is_same_v<std::remove_cvref_t<Self>, T>;

/// @brief Wraps the constructor of `T` into the erased one.
template <typename T, typename... Args>
UsertypeData::Constructor makeConstructor(std::function<T(Args...)> function) {
    static_assert(kValidParams<T, Args...>);
    return [function = std::move(function)](void* storage, const std::span<const Value> args) {
        applyArgs<Args...>(
            args, [&](auto&&... values) { ::new (storage) T(function(std::forward<decltype(values)>(values)...)); });
    };
}

/// @brief Wraps the getter of a member of `T` into the erased one.
template <typename T, typename R, typename Self>
UsertypeData::Getter makeGetter(std::function<R(Self)> function) {
    static_assert(kIsSelf<T, Self>, "the parameter of a getter must be the object, as T& or const T&");
    return [function = std::move(function)](void* self) -> Value { return toValue(function(*static_cast<T*>(self))); };
}

/// @brief Wraps a method of `T` into the erased one.
template <typename T, typename R, typename Self, typename... Rest>
UsertypeData::Method makeMethod(std::function<R(Self, Rest...)> function) {
    static_assert(kIsSelf<T, Self>, "the first parameter of a method must be the object, as T& or const T&");
    static_assert(kValidParams<R, Rest...>);
    return [function = std::move(function)](void* self, const std::span<const Value> args) -> ValuePack {
        return applyArgs<Rest...>(args, [&](auto&&... values) -> ValuePack {
            T& object = *static_cast<T*>(self);
            if constexpr (std::is_void_v<R>) {
                function(object, std::forward<decltype(values)>(values)...);
                return {};
            } else {
                return toPack(function(object, std::forward<decltype(values)>(values)...));
            }
        });
    };
}

/// @brief Wraps the setter of a member of `T` into the erased one.
template <typename T, typename Self, typename Param>
UsertypeData::Setter makeSetter(std::function<void(Self, Param)> function) {
    return [method = makeMethod<T>(std::move(function))](void* self, const Value& value) {
        // A setter takes exactly one argument, so the method never gives anything back.
        (void)method(self, std::span<const Value>{&value, 1});
    };
}
}  // namespace detail

template <typename T>
Usertype<T>::Usertype(detail::UsertypeData& data) noexcept : _data{&data} {
    static_assert(alignof(T) <= alignof(double), "Luau does not align userdata more than a double");
}

template <typename T>
const std::string& Usertype<T>::name() const noexcept {
    return _data->name();
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::constructor(F&& function) {
    _data->setConstructor(detail::makeConstructor<T>(std::function{std::forward<F>(function)}));
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::getter(std::string name, F&& function) {
    _data->addGetter(std::move(name), detail::makeGetter<T>(std::function{std::forward<F>(function)}));
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::setter(std::string name, F&& function) {
    _data->addSetter(std::move(name), detail::makeSetter<T>(std::function{std::forward<F>(function)}));
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::method(std::string name, F&& function) {
    _data->addMethod(std::move(name), detail::makeMethod<T>(std::function{std::forward<F>(function)}));
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::index(F&& function) {
    _data->setIndexFallback(
        [function = std::function<Value(T&, std::string_view)>{std::forward<F>(function)}](
            void* self, const std::string_view key) -> Value { return function(*static_cast<T*>(self), key); });
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::newIndex(F&& function) {
    _data->setNewIndexFallback([function = std::function<void(T&, std::string_view, const Value&)>{std::forward<F>(
                                    function)}](void* self, const std::string_view key, const Value& value) {
        function(*static_cast<T*>(self), key, value);
    });
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::eq(F&& function) {
    _data->setEquality([function = std::function<bool(const T&, const T&)>{std::forward<F>(function)}](
                           const void* lhs, const void* rhs) {
        return function(*static_cast<const T*>(lhs), *static_cast<const T*>(rhs));
    });
    return *this;
}

template <typename T>
template <typename F>
Usertype<T>& Usertype<T>::tostring(F&& function) {
    _data->setStringifier([function = std::function<std::string(const T&)>{std::forward<F>(function)}](
                              const void* self) { return function(*static_cast<const T*>(self)); });
    return *this;
}

template <typename T>
Result<Userdata> Usertype<T>::create(T value) const {
    return _data->create([](void* storage, void* source) { ::new (storage) T(std::move(*static_cast<T*>(source))); },
                         &value);
}
}  // namespace rtype::luau
