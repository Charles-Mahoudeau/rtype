/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#pragma once

#ifndef RTYPE_RUNTIME_INCLUDING_TPP
#error "Runtime.tpp is an implementation detail of Runtime.hpp: include Runtime.hpp instead"
#endif

#include <string>
#include <typeinfo>
#include <utility>

#include "Exceptions.hpp"
#include "Usertype.hpp"
#include "detail/UsertypeRegistry.hpp"

namespace rtype::luau {
template <typename T>
Usertype<T> Runtime::usertype(std::string name) {
    return Usertype<T>{usertypes().add(typeid(T), std::move(name), sizeof(T), &detail::destroyUserdata<T>)};
}

template <typename T>
Usertype<T> Runtime::usertype() const {
    detail::UsertypeData* const data{usertypes().find(typeid(T))};
    if (data == nullptr) {
        throw exceptions::InvalidOperation{"the type is not registered as a usertype of this runtime"};
    }
    return Usertype<T>{*data};
}
}  // namespace rtype::luau
