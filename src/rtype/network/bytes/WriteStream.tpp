/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WriteStream
*/

#pragma once

namespace rtype::network::bytes {
namespace detail {
/// @brief Writes an unsigned integer through the `ByteWriter` function of its size.
inline void writeUnsigned(ByteWriter& writer, const std::uint8_t value) noexcept { writer.writeU8(value); }

inline void writeUnsigned(ByteWriter& writer, const std::uint16_t value) noexcept { writer.writeU16(value); }

inline void writeUnsigned(ByteWriter& writer, const std::uint32_t value) noexcept { writer.writeU32(value); }

inline void writeUnsigned(ByteWriter& writer, const std::uint64_t value) noexcept { writer.writeU64(value); }

/// @brief Writes the bits of a `WireValue` as an unsigned integer of the same size.
template <typename T>
void writeScalar(ByteWriter& writer, const T value) noexcept {
    writeUnsigned(writer, std::bit_cast<UnsignedOfSize<T>>(value));
}
}  // namespace detail

template <WireValue T>
void WriteStream::value(const T& value) noexcept {
    if (begin()) {
        detail::writeScalar(_writer, value);
    }
}

template <WireValue T>
void WriteStream::constant(const T expected) noexcept {
    value(expected);
}

template <WireCount Count, typename T>
void WriteStream::list(const std::vector<T>& items, const std::size_t maxCount) noexcept {
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
