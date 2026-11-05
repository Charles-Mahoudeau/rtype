/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Device
*/

#pragma once

#ifndef RTYPE_DEVICE_INCLUDING_TPP
#error "Device.tpp is an implementation detail of Device.hpp: include Device.hpp instead"
#endif

#include <bit>
#include <cstdint>
#include <string_view>

namespace rtype::render::vulkan::core {

template <typename Handle>
std::uint64_t objectHandleOf(Handle handle) noexcept {
    return std::bit_cast<std::uint64_t>(static_cast<Handle::CType>(handle));
}

template <typename Handle>
void Device::setName(Handle handle, std::string_view name) const {
    if (_debugUtils) {
        setObjectName(Handle::objectType, objectHandleOf(handle), name);
    }
}

}  // namespace rtype::render::vulkan::core
