/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ViewAs
*/

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

namespace rtype::ecs::storage {
template <typename T>
T* viewAs(std::span<std::byte> bytes) noexcept {
    assert(bytes.size() == sizeof(T));
    assert(std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(T) == 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the value already lives in these bytes
    return reinterpret_cast<T*>(bytes.data());
}

template <typename T>
const T* viewAs(std::span<const std::byte> bytes) noexcept {
    assert(bytes.size() == sizeof(T));
    assert(std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(T) == 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the value already lives in these bytes
    return reinterpret_cast<const T*>(bytes.data());
}
}  // namespace rtype::ecs::storage
