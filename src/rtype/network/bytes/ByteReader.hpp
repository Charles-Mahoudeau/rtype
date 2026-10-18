/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteReader
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "rtype/network/Export.hpp"

namespace rtype::network::bytes {
/// @brief Reads little-endian values from a buffer owned by the caller, without ever reading past its end.
///
/// @details A read that needs more bytes than remain returns 0 (or an empty span or string), consumes nothing and
/// makes the reader invalid for good: every later read returns 0 even if bytes remain. A parser reads every field it
/// expects and checks `isValid()` once at the end. It never throws and never touches memory outside the buffer.
class RTYPE_NETWORK_API ByteReader {
  public:
    /// @brief Builds a reader positioned at the start of `data`.
    /// @param data The source, owned by the caller and alive as long as the reader (and its views) are used.
    explicit ByteReader(std::span<const std::byte> data) noexcept;

    /// @brief Destroys the reader; the caller's buffer is left untouched.
    ~ByteReader() = default;

    /// @brief Copies the reader; the copy reads the same buffer, with its own position and validity.
    ByteReader(const ByteReader&) = default;

    /// @brief Copy-assigns the reader; see the copy constructor.
    ByteReader& operator=(const ByteReader&) = default;

    /// @brief Moves the reader.
    ByteReader(ByteReader&&) noexcept = default;

    /// @brief Move-assigns the reader.
    ByteReader& operator=(ByteReader&&) noexcept = default;

    /// @brief Reads one byte; 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::uint8_t readU8() noexcept;

    /// @brief Reads a 16-bit unsigned integer, lowest byte first; 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::uint16_t readU16() noexcept;

    /// @brief Reads a 32-bit unsigned integer, lowest byte first; 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::uint32_t readU32() noexcept;

    /// @brief Reads a 64-bit unsigned integer, lowest byte first; 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::uint64_t readU64() noexcept;

    /// @brief Reads a signed byte (two's complement); 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::int8_t readI8() noexcept;

    /// @brief Reads a 16-bit signed integer (two's complement); 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::int16_t readI16() noexcept;

    /// @brief Reads a 32-bit signed integer (two's complement); 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::int32_t readI32() noexcept;

    /// @brief Reads a 64-bit signed integer (two's complement); 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] std::int64_t readI64() noexcept;

    /// @brief Reads the IEEE 754 bits of a float, kept bit for bit; 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] float readF32() noexcept;

    /// @brief Reads the IEEE 754 bits of a double, kept bit for bit; 0 once invalid.
    /// @return The value read, or 0 once invalid.
    [[nodiscard]] double readF64() noexcept;

    /// @brief Reads `count` bytes without copying them.
    /// @param count How many bytes to read.
    /// @return A view into the caller's buffer, only valid while that buffer lives; empty once invalid.
    [[nodiscard]] std::span<const std::byte> readBytes(std::size_t count) noexcept;

    /// @brief Reads a `u8` length, then that many bytes. The bytes are not checked as UTF-8.
    /// @return The text, or an empty string once invalid.
    [[nodiscard]] std::string readString() noexcept;

    /// @brief Moves forward by `count` bytes without reading them; past the end, it makes the reader invalid.
    /// @param count How many bytes to skip.
    void skip(std::size_t count) noexcept;

    /// @brief Tells whether every read so far fitted.
    /// @return False after the first read past the end.
    [[nodiscard]] bool isValid() const noexcept;

    /// @brief Gives the number of bytes not read yet.
    /// @return The number of unread bytes.
    [[nodiscard]] std::size_t remainingSize() const noexcept;

    /// @brief Tells whether the reader is valid and nothing remains; decoders use it to reject trailing bytes.
    /// @return True when valid and fully consumed.
    [[nodiscard]] bool isFinished() const noexcept;

  private:
    /// @brief Consumes the next `count` bytes of the buffer.
    /// @param count How many bytes to consume.
    /// @return The consumed region, or an empty span (and an invalid reader) when the reader is already invalid or
    /// fewer than `count` bytes remain. A `count` of 0 also gives an empty span and changes nothing.
    [[nodiscard]] std::span<const std::byte> claim(std::size_t count) noexcept;

    std::span<const std::byte> _data;  ///< Owned by the caller.
    std::size_t _position{0};          ///< Next byte to read.
    bool _valid{true};                 ///< False after the first read past the end.
};
}  // namespace rtype::network::bytes
