/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteWriter
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "rtype/network/Export.hpp"

namespace rtype::network::bytes {
/// @brief Writes little-endian values into a buffer owned by the caller.
///
/// @details It never allocates and never grows the buffer. A write that does not fit writes nothing and makes the
/// writer invalid for good: every later write does nothing, so an encoder can write a whole message and check
/// `isValid()` once at the end.
class RTYPE_NETWORK_API ByteWriter {
  public:
    /// @brief Builds a writer that fills `buffer` from its start.
    /// @param buffer The destination, owned by the caller and alive as long as the writer is used.
    explicit ByteWriter(std::span<std::byte> buffer) noexcept;

    /// @brief Destroys the writer; the caller's buffer is left untouched.
    ~ByteWriter() = default;

    /// @brief Copies the writer; the copy writes into the same buffer, with its own position and validity.
    ByteWriter(const ByteWriter&) = default;

    /// @brief Copy-assigns the writer; see the copy constructor.
    ByteWriter& operator=(const ByteWriter&) = default;

    /// @brief Moves the writer.
    ByteWriter(ByteWriter&&) noexcept = default;

    /// @brief Move-assigns the writer.
    ByteWriter& operator=(ByteWriter&&) noexcept = default;

    /// @brief Writes one byte.
    /// @param value The value to write.
    void writeU8(std::uint8_t value) noexcept;

    /// @brief Writes a 16-bit unsigned integer, lowest byte first.
    /// @param value The value to write.
    void writeU16(std::uint16_t value) noexcept;

    /// @brief Writes a 32-bit unsigned integer, lowest byte first.
    /// @param value The value to write.
    void writeU32(std::uint32_t value) noexcept;

    /// @brief Writes a 64-bit unsigned integer, lowest byte first.
    /// @param value The value to write.
    void writeU64(std::uint64_t value) noexcept;

    /// @brief Writes a signed byte as the unsigned value of the same size (two's complement).
    /// @param value The value to write.
    void writeI8(std::int8_t value) noexcept;

    /// @brief Writes a 16-bit signed integer as the unsigned value of the same size (two's complement).
    /// @param value The value to write.
    void writeI16(std::int16_t value) noexcept;

    /// @brief Writes a 32-bit signed integer as the unsigned value of the same size (two's complement).
    /// @param value The value to write.
    void writeI32(std::int32_t value) noexcept;

    /// @brief Writes a 64-bit signed integer as the unsigned value of the same size (two's complement).
    /// @param value The value to write.
    void writeI64(std::int64_t value) noexcept;

    /// @brief Writes the IEEE 754 bits of a float; a NaN or `-0.0` keeps its exact bits and is not checked.
    /// @param value The value to write.
    void writeF32(float value) noexcept;

    /// @brief Writes the IEEE 754 bits of a double; a NaN or `-0.0` keeps its exact bits and is not checked.
    /// @param value The value to write.
    void writeF64(double value) noexcept;

    /// @brief Writes one raw byte.
    /// @param byte The byte to write.
    void writeByte(std::byte byte) noexcept;

    /// @brief Writes raw bytes as they are.
    /// @param bytes The bytes to write.
    void writeBytes(std::span<const std::byte> bytes) noexcept;

    /// @brief Writes the length of `text` as a `u8`, then its bytes. A text longer than 255 bytes writes nothing and
    /// makes the writer invalid.
    /// @param text The text to write; its bytes are not checked as UTF-8.
    void writeString(std::string_view text) noexcept;

    /// @brief Writes `count` zero bytes (handshake padding).
    /// @param count How many zero bytes to write.
    void writeZeros(std::size_t count) noexcept;

    /// @brief Tells whether every write so far fitted.
    /// @return False after the first write that did not fit.
    [[nodiscard]] bool isValid() const noexcept;

    /// @brief Gives the number of bytes written; it stays at the size reached before a failure.
    /// @return The number of bytes written.
    [[nodiscard]] std::size_t bytesWritten() const noexcept;

    /// @brief Gives the bytes written so far, as a view of the caller's buffer.
    /// @return The written prefix of the buffer.
    [[nodiscard]] std::span<const std::byte> writtenBytes() const noexcept;

  private:
    /// @brief Reserves the next `count` bytes of the buffer.
    /// @param count How many bytes to reserve.
    /// @return The reserved region, or an empty span (and an invalid writer) when the writer is already invalid or
    /// the bytes do not fit. A `count` of 0 also gives an empty span and changes nothing.
    [[nodiscard]] std::span<std::byte> claim(std::size_t count) noexcept;

    std::span<std::byte> _buffer;  ///< Owned by the caller.
    std::size_t _written{0};       ///< Bytes written so far.
    bool _valid{true};             ///< False after the first write that did not fit.
};
}  // namespace rtype::network::bytes
