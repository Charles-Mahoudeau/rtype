/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReadStream
*/

#include <bit>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace rtype::network::bytes {
namespace detail {
/// @brief Reads an unsigned integer of the size given by the tag; the overload is picked by the tag's type.
[[nodiscard]] inline std::uint8_t readUnsigned(ByteReader& reader, std::type_identity<std::uint8_t> /*tag*/) noexcept {
    return reader.readU8();
}

[[nodiscard]] inline std::uint16_t readUnsigned(ByteReader& reader,
                                                std::type_identity<std::uint16_t> /*tag*/) noexcept {
    return reader.readU16();
}

[[nodiscard]] inline std::uint32_t readUnsigned(ByteReader& reader,
                                                std::type_identity<std::uint32_t> /*tag*/) noexcept {
    return reader.readU32();
}

[[nodiscard]] inline std::uint64_t readUnsigned(ByteReader& reader,
                                                std::type_identity<std::uint64_t> /*tag*/) noexcept {
    return reader.readU64();
}

/// @brief Reads the bits of a `WireValue` as an unsigned integer of the same size, then reinterprets them as `T`;
/// the enum range is not checked here.
template <typename T>
[[nodiscard]] T readScalar(ByteReader& reader) noexcept {
    return std::bit_cast<T>(readUnsigned(reader, std::type_identity<UnsignedOfSize<T>>{}));
}
}  // namespace detail

template <WireValue T>
void ReadStream::value(T& value) noexcept {
    if (!begin()) {
        return;
    }
    if constexpr (std::is_same_v<T, bool>) {
        // A bool is one byte that must be exactly 0 or 1, so read it raw.
        const std::uint8_t raw = _reader.readU8();
        if (raw > 1 || !_reader.isValid()) {
            _valid = false;
            return;
        }
        value = raw == 1;
    } else {
        const T read = detail::readScalar<T>(_reader);
        if (!_reader.isValid()) {
            _valid = false;
            return;
        }
        if constexpr (std::is_enum_v<T>) {
            if (!isValidWireValue(read)) {
                _valid = false;
                return;
            }
        }
        value = read;
    }
}

template <WireValue T>
void ReadStream::constant(const T expected) noexcept {
    T actual{};
    value(actual);
    if (isValid() && actual != expected) {
        _valid = false;
    }
}

template <WireCount Count, typename T>
void ReadStream::list(std::vector<T>& items, const std::size_t maxCount) noexcept {
    items.clear();
    Count count{};
    value(count);
    if (!isValid()) {
        return;
    }
    if (count > maxCount) {
        _valid = false;
        return;
    }
    items.reserve(count);
    for (Count index = 0; index < count; ++index) {
        T item{};
        if constexpr (requires { T::serialize(item, *this); }) {
            T::serialize(item, *this);
        } else {
            value(item);
        }
        if (!isValid()) {
            return;
        }
        items.push_back(std::move(item));
    }
}
}  // namespace rtype::network::bytes
