/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteWriter
*/

#include "rtype/network/bytes/ByteWriter.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

namespace rtype::network::bytes {
namespace {
static_assert(sizeof(float) == sizeof(std::uint32_t) && std::numeric_limits<float>::is_iec559,
              "float must be IEEE 754 binary32");
static_assert(sizeof(double) == sizeof(std::uint64_t) && std::numeric_limits<double>::is_iec559,
              "double must be IEEE 754 binary64");

constexpr std::size_t kMaxStringSize = std::numeric_limits<std::uint8_t>::max();

/// @brief Fills `region` with the lowest bytes of `value`, lowest byte first.
void storeLittleEndian(const std::span<std::byte> region, std::uint64_t value) noexcept {
    for (std::byte& slot : region) {
        slot = std::byte{static_cast<std::uint8_t>(value & 0xFFU)};
        value >>= 8U;
    }
}
}  // namespace

ByteWriter::ByteWriter(const std::span<std::byte> buffer) noexcept : _buffer{buffer} {}

void ByteWriter::writeU8(const std::uint8_t value) noexcept { storeLittleEndian(claim(sizeof(value)), value); }

void ByteWriter::writeU16(const std::uint16_t value) noexcept { storeLittleEndian(claim(sizeof(value)), value); }

void ByteWriter::writeU32(const std::uint32_t value) noexcept { storeLittleEndian(claim(sizeof(value)), value); }

void ByteWriter::writeU64(const std::uint64_t value) noexcept { storeLittleEndian(claim(sizeof(value)), value); }

void ByteWriter::writeI8(const std::int8_t value) noexcept { writeU8(static_cast<std::uint8_t>(value)); }

void ByteWriter::writeI16(const std::int16_t value) noexcept { writeU16(static_cast<std::uint16_t>(value)); }

void ByteWriter::writeI32(const std::int32_t value) noexcept { writeU32(static_cast<std::uint32_t>(value)); }

void ByteWriter::writeI64(const std::int64_t value) noexcept { writeU64(static_cast<std::uint64_t>(value)); }

void ByteWriter::writeF32(const float value) noexcept { writeU32(std::bit_cast<std::uint32_t>(value)); }

void ByteWriter::writeF64(const double value) noexcept { writeU64(std::bit_cast<std::uint64_t>(value)); }

void ByteWriter::writeByte(const std::byte byte) noexcept {
    if (const std::span<std::byte> region = claim(1); !region.empty()) {
        region.front() = byte;
    }
}

void ByteWriter::writeBytes(const std::span<const std::byte> bytes) noexcept {
    if (const std::span<std::byte> region = claim(bytes.size()); !region.empty()) {
        std::ranges::copy(bytes, region.begin());
    }
}

void ByteWriter::writeString(const std::string_view text) noexcept {
    if (text.size() > kMaxStringSize) {
        _valid = false;
        return;
    }
    const std::span<std::byte> region = claim(text.size() + 1);
    if (region.empty()) {
        return;
    }
    region.front() = std::byte{static_cast<std::uint8_t>(text.size())};
    std::ranges::copy(std::as_bytes(std::span{text}), region.subspan(1).begin());
}

void ByteWriter::writeZeros(const std::size_t count) noexcept { std::ranges::fill(claim(count), std::byte{0}); }

bool ByteWriter::isValid() const noexcept { return _valid; }

std::size_t ByteWriter::bytesWritten() const noexcept { return _written; }

std::span<const std::byte> ByteWriter::writtenBytes() const noexcept { return _buffer.first(_written); }

std::span<std::byte> ByteWriter::claim(const std::size_t count) noexcept {
    if (!_valid || count > _buffer.size() - _written) {
        _valid = false;
        return {};
    }
    const std::span<std::byte> region = _buffer.subspan(_written, count);
    _written += count;
    return region;
}
}  // namespace rtype::network::bytes
