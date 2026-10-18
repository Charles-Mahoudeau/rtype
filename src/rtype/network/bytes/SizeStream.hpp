/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SizeStream
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/bytes/WireValue.hpp"

namespace rtype::network::bytes {
/// @brief Third side of a `serialize` function: it writes nothing and counts the bytes `WriteStream` would write.
///
/// @details It has the same function names and the same limits as `WriteStream`, so `Codec::encode` can know the
/// exact size of a message before allocating it.
class RTYPE_NETWORK_API SizeStream {
  public:
    /// @brief Builds a stream that has already counted `initialSize` bytes.
    /// @param initialSize Bytes written before the stream (the kind byte).
    explicit SizeStream(std::size_t initialSize = 0) noexcept;
    /// @brief Destroys the stream.
    ~SizeStream() = default;
    /// @brief Copies the stream.
    SizeStream(const SizeStream&) = default;
    /// @brief Copy-assigns the stream.
    SizeStream& operator=(const SizeStream&) = default;
    /// @brief Moves the stream.
    SizeStream(SizeStream&&) noexcept = default;
    /// @brief Move-assigns the stream.
    SizeStream& operator=(SizeStream&&) noexcept = default;

    /// @brief Counts the bytes of `WriteStream::value`.
    template <WireValue T>
    void value(const T& value) noexcept;
    /// @brief Counts the bytes of `WriteStream::constant`.
    template <WireValue T>
    void constant(T expected) noexcept;
    /// @brief Counts the bytes of `WriteStream::bytes`.
    /// @param bytes The bytes that would be written.
    void bytes(std::span<const std::byte> bytes) noexcept;
    /// @brief Counts the bytes of `WriteStream::string`.
    /// @param text The text that would be written.
    /// @param maxLength The longest accepted text.
    void string(const std::string& text, std::size_t maxLength) noexcept;
    /// @brief Counts the bytes of `WriteStream::list`.
    /// @param items The items that would be written.
    /// @param maxCount The largest accepted number of items.
    template <WireCount Count, typename T>
    void list(const std::vector<T>& items, std::size_t maxCount) noexcept;
    /// @brief Counts the bytes of `WriteStream::padTo`.
    /// @param size The wanted size; invalid if already longer.
    void padTo(std::size_t size) noexcept;
    /// @brief Counts the bytes of `WriteStream::rest`.
    /// @param bytes The bytes that would be written.
    void rest(std::span<const std::byte> bytes) noexcept;

    /// @brief Tells whether `WriteStream` would accept every call so far.
    /// @return False once a limit is broken.
    [[nodiscard]] bool isValid() const noexcept;
    /// @brief Gives the number of bytes counted.
    /// @return The size of the message so far.
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    /// @brief Starts a call: invalidates the stream if `rest` was already called.
    /// @return True if the call may go on.
    [[nodiscard]] bool begin() noexcept;

    std::size_t _size;      ///< Bytes counted.
    bool _valid{true};      ///< False after the first broken limit.
    bool _finished{false};  ///< True after `rest`.
};
}  // namespace rtype::network::bytes

#include "SizeStream.tpp"  // IWYU pragma: export
