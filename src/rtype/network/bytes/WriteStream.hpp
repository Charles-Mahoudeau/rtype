/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WriteStream
*/

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/bytes/WireValue.hpp"

namespace rtype::network::bytes {
/// @brief Writing side of a `serialize` function; `ReadStream` has the same function names.
///
/// @details Every call does nothing once the stream is invalid, so a `serialize` lists its fields and the caller
/// checks `isValid()` once at the end. A stream becomes invalid when the writer does, when a value breaks a limit
/// (`string`, `list`, `padTo`), or when a call follows `rest`.
class RTYPE_NETWORK_API WriteStream {
  public:
    /// @brief Builds a stream over `writer`, which must already hold the message's kind byte if it has one.
    /// @param writer The destination, alive as long as the stream is used.
    explicit WriteStream(ByteWriter& writer) noexcept;
    /// @brief Destroys the stream.
    ~WriteStream() = default;
    /// @brief A stream is bound to its writer: it cannot be copied.
    WriteStream(const WriteStream&) = delete;
    /// @brief A stream is bound to its writer: it cannot be copy-assigned.
    WriteStream& operator=(const WriteStream&) = delete;
    /// @brief A stream is bound to its writer: it cannot be moved.
    WriteStream(WriteStream&&) = delete;
    /// @brief A stream is bound to its writer: it cannot be move-assigned.
    WriteStream& operator=(WriteStream&&) = delete;

    /// @brief Writes an integer, float, `bool` (`u8` 0 or 1) or wire enum (its underlying integer).
    /// @param value The value to write.
    template <WireValue T>
    void value(const T& value) noexcept;
    /// @brief Writes `expected`; on the reading side, any other value makes the stream invalid.
    /// @param expected The constant.
    template <WireValue T>
    void constant(T expected) noexcept;
    /// @brief Writes raw bytes whose size both sides know.
    /// @param bytes The bytes to write.
    void bytes(std::span<const std::byte> bytes) noexcept;
    /// @brief Writes a `u8` length, then the bytes of `text`; invalid if `text` is longer than `maxLength`.
    /// @param text The text to write.
    /// @param maxLength The longest accepted text (at most 255).
    void string(const std::string& text, std::size_t maxLength) noexcept;
    /// @brief Writes the number of items as a `Count`, then each item; invalid if there are more than `maxCount`.
    /// @details An item that is a struct with a `serialize` function is written with it, any other with `value`.
    /// @param items The items to write.
    /// @param maxCount The largest accepted number of items.
    template <WireCount Count, typename T>
    void list(const std::vector<T>& items, std::size_t maxCount) noexcept;
    /// @brief Writes zeros until the message (kind byte included) is `size` bytes long.
    /// @param size The wanted size; invalid if the message is already longer.
    void padTo(std::size_t size) noexcept;
    /// @brief Writes a trailing payload; it must be the last call.
    /// @param bytes The bytes to write.
    void rest(std::span<const std::byte> bytes) noexcept;

    /// @brief Tells whether every call so far was valid and fitted.
    /// @return False once the stream is invalid.
    [[nodiscard]] bool isValid() const noexcept;

  private:
    /// @brief Starts a call: invalidates the stream if `rest` was already called.
    /// @return True if the call may go on.
    [[nodiscard]] bool begin() noexcept;

    ByteWriter& _writer;    ///< Owned by the caller.
    bool _valid{true};      ///< False after the first broken limit.
    bool _finished{false};  ///< True after `rest`.
};
}  // namespace rtype::network::bytes

#include "WriteStream.tpp"  // IWYU pragma: export
