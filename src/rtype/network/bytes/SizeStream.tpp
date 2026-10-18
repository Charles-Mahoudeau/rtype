/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SizeStream
*/

#pragma once

namespace rtype::network::bytes {
template <WireValue T>
void SizeStream::value(const T& /*value*/) noexcept {
    if (begin()) {
        _size += sizeof(T);
    }
}

template <WireValue T>
void SizeStream::constant(const T expected) noexcept {
    value(expected);
}

template <WireCount Count, typename T>
void SizeStream::list(const std::vector<T>& items, const std::size_t maxCount) noexcept {
    if (!begin()) {
        return;
    }
    if (items.size() > maxCount || items.size() > std::numeric_limits<Count>::max()) {
        _valid = false;
        return;
    }
    value(static_cast<Count>(items.size()));
    for (const T& item : items) {
        if constexpr (requires { T::serialize(item, *this); }) {
            T::serialize(item, *this);
        } else {
            value(item);
        }
    }
}
}  // namespace rtype::network::bytes
