/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteReader
*/

#include "rtype/network/bytes/ByteReader.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace rtype::network::bytes {
namespace {
/// @brief Builds an integer from `region`, lowest byte first; an empty region gives 0.
[[nodiscard]] std::uint64_t loadLittleEndian(const std::span<const std::byte> region) noexcept {
    std::uint64_t value = 0;
    std::uint8_t shift = 0;
    for (const std::byte slot : region) {
        value |= static_cast<std::uint64_t>(slot) << shift;
        shift += 8U;
    }
    return value;
}
}  // namespace

ByteReader::ByteReader(const std::span<const std::byte> data) noexcept : _data{data} {}

std::uint8_t ByteReader::readU8() noexcept { return static_cast<std::uint8_t>(loadLittleEndian(claim(1))); }

std::uint16_t ByteReader::readU16() noexcept { return static_cast<std::uint16_t>(loadLittleEndian(claim(2))); }

std::uint32_t ByteReader::readU32() noexcept { return static_cast<std::uint32_t>(loadLittleEndian(claim(4))); }

std::uint64_t ByteReader::readU64() noexcept { return loadLittleEndian(claim(8)); }

std::int8_t ByteReader::readI8() noexcept { return static_cast<std::int8_t>(readU8()); }

std::int16_t ByteReader::readI16() noexcept { return static_cast<std::int16_t>(readU16()); }

std::int32_t ByteReader::readI32() noexcept { return static_cast<std::int32_t>(readU32()); }

std::int64_t ByteReader::readI64() noexcept { return static_cast<std::int64_t>(readU64()); }

float ByteReader::readF32() noexcept { return std::bit_cast<float>(readU32()); }

double ByteReader::readF64() noexcept { return std::bit_cast<double>(readU64()); }

std::span<const std::byte> ByteReader::readBytes(const std::size_t count) noexcept { return claim(count); }

std::string ByteReader::readString() noexcept {
    if (!_valid || _position >= _data.size()) {
        _valid = false;
        return {};
    }
    const auto length = static_cast<std::size_t>(std::to_integer<std::uint8_t>(_data.subspan(_position).front()));
    const std::span<const std::byte> region = claim(length + 1);
    if (region.empty()) {
        return {};
    }
    std::string text;
    text.resize(length);
    std::ranges::transform(region.subspan(1), text.begin(),
                           [](const std::byte byte) { return static_cast<char>(byte); });
    return text;
}

void ByteReader::skip(const std::size_t count) noexcept { static_cast<void>(claim(count)); }

bool ByteReader::isValid() const noexcept { return _valid; }

std::size_t ByteReader::remainingSize() const noexcept { return _data.size() - _position; }

bool ByteReader::isFinished() const noexcept { return _valid && _position == _data.size(); }

std::span<const std::byte> ByteReader::claim(const std::size_t count) noexcept {
    if (!_valid || count > _data.size() - _position) {
        _valid = false;
        return {};
    }
    const std::span<const std::byte> region = _data.subspan(_position, count);
    _position += count;
    return region;
}
}  // namespace rtype::network::bytes
