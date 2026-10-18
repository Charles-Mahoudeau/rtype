/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReadStream
*/

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/WireValue.hpp"

namespace rtype::network::bytes {
/// @brief Reading side of a `serialize` function; `WriteStream` has the same function names.
///
/// @details Every call does nothing once the stream is invalid, so a `serialize` lists its fields and the caller
/// checks `isValid()` once at the end. Every value is checked as it is read: a `bool` must be 0 or 1, an enum must
/// pass `isValidWireValue`, a constant must match, and strings and lists must respect their limits. A stream also
/// becomes invalid when bytes are missing or when a call follows `rest`.
class RTYPE_NETWORK_API ReadStream {
  public:
    /// @brief Builds a stream over `reader`.
    /// @param reader The source, alive as long as the stream is used.
    /// @param consumed Bytes of the message already read from the reader (the kind byte), so that `padTo` counts
    /// the message from its first byte.
    explicit ReadStream(ByteReader& reader, std::size_t consumed = 0) noexcept;
    /// @brief Destroys the stream.
    ~ReadStream() = default;
    /// @brief A stream is bound to its reader: it cannot be copied.
    ReadStream(const ReadStream&) = delete;
    /// @brief A stream is bound to its reader: it cannot be copy-assigned.
    ReadStream& operator=(const ReadStream&) = delete;
    /// @brief A stream is bound to its reader: it cannot be moved.
    ReadStream(ReadStream&&) = delete;
    /// @brief A stream is bound to its reader: it cannot be move-assigned.
    ReadStream& operator=(ReadStream&&) = delete;

    /// @brief Reads an integer, float, `bool` (`u8`, 0 or 1) or wire enum (checked with `isValidWireValue`).
    /// @param value Receives the value; left unchanged if the read fails or the value is rejected.
    template <WireValue T>
    void value(T& value) noexcept;
    /// @brief Reads a value of the type of `expected`; the stream becomes invalid if it differs.
    /// @param expected The constant.
    template <WireValue T>
    void constant(T expected) noexcept;
    /// @brief Fills `destination` with raw bytes; invalid if fewer remain.
    /// @param destination The bytes to fill; its size is known by both sides.
    void bytes(std::span<std::byte> destination) noexcept;
    /// @brief Reads a `u8` length, then that many bytes; invalid if the length is above `maxLength`.
    /// @param text Receives the text.
    /// @param maxLength The longest accepted text.
    void string(std::string& text, std::size_t maxLength) noexcept;
    /// @brief Reads the number of items as a `Count`, then each item.
    /// @details The count is checked against `maxCount` before anything is allocated. An item that is a struct with a
    /// `serialize` function is read with it, any other with `value`.
    /// @param items Receives the items (cleared first).
    /// @param maxCount The largest accepted number of items.
    template <WireCount Count, typename T>
    void list(std::vector<T>& items, std::size_t maxCount) noexcept;
    /// @brief Skips bytes until the message (kind byte included) is `size` bytes long; their content is not checked.
    /// @param size The wanted size; invalid if the message is shorter or already longer.
    void padTo(std::size_t size) noexcept;
    /// @brief Sets `bytes` to every remaining byte (possibly none), as a view into the input; it must be the last call.
    /// @param bytes Receives the view, valid while the input buffer lives.
    void rest(std::span<const std::byte>& bytes) noexcept;

    /// @brief Tells whether every call so far was valid.
    /// @return False once the stream is invalid.
    [[nodiscard]] bool isValid() const noexcept;

  private:
    /// @brief Starts a call: invalidates the stream if `rest` was already called.
    /// @return True if the call may go on.
    [[nodiscard]] bool begin() noexcept;

    ByteReader& _reader;            ///< Owned by the caller.
    std::size_t _consumed;          ///< Bytes of the message read before the stream was built.
    std::size_t _initialRemaining;  ///< Bytes `_reader` had left when the stream was built.
    bool _valid{true};              ///< False after the first rejected value.
    bool _finished{false};          ///< True after `rest`.
};
}  // namespace rtype::network::bytes

#include "ReadStream.tpp"  // IWYU pragma: export
