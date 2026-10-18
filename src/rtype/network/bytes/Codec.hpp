/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Codec
*/

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/bytes/ReadStream.hpp"
#include "rtype/network/bytes/SizeStream.hpp"
#include "rtype/network/bytes/WriteStream.hpp"

namespace rtype::network::bytes {
/// @brief A message of a codec: default-constructible, with a `kKind` and a `serialize` function listing its fields
/// once, for writing (`const M&` and a `WriteStream`), counting (`SizeStream`) and reading (`M&` and a `ReadStream`).
template <typename M, typename Kind>
concept CodecMessage =
    std::is_default_constructible_v<M> &&
    requires(const M& constMessage, M& message, WriteStream& out, SizeStream& count, ReadStream& in) {
        { M::kKind } -> std::convertible_to<Kind>;
        M::serialize(constMessage, out);
        M::serialize(constMessage, count);
        M::serialize(message, in);
    };

/// @brief Tells whether no two messages share the same `kKind`.
template <typename Kind, typename... Messages>
[[nodiscard]] constexpr bool kHasDistinctKinds() noexcept {
    const std::array<std::uint8_t, sizeof...(Messages)> kinds{static_cast<std::uint8_t>(Messages::kKind)...};
    for (std::size_t first = 0; first < kinds.size(); ++first) {
        for (std::size_t second = first + 1; second < kinds.size(); ++second) {
            if (kinds[first] == kinds[second]) {
                return false;
            }
        }
    }
    return true;
}

/// @brief Encodes and decodes a closed family of messages: a kind byte, then the message's fields.
template <typename Kind, typename Variant>
class Codec;

/// @brief Encodes and decodes a closed family of messages: a kind byte, then the message's fields.
///
/// @details The family is the list of alternatives of the variant. `decode` finds the type of a datagram with one
/// lookup in a table built at compile time. Nothing here throws on bad input or reads outside the input.
/// @tparam Kind The enum of the kind byte.
/// @tparam Messages The messages of the family, each a `CodecMessage` with its own `kKind`.
template <typename Kind, typename... Messages>
class Codec<Kind, std::variant<Messages...>> {
    static_assert((CodecMessage<Messages, Kind> && ...),
                  "Codec: every message needs a default constructor, a kKind and a serialize function usable with "
                  "WriteStream, SizeStream and ReadStream");
    static_assert(kHasDistinctKinds<Kind, Messages...>(), "Codec: two messages of the family share the same kKind");

  public:
    /// @brief The family as a variant.
    using Message = std::variant<Messages...>;

    /// @brief Writes the kind byte, then the fields of `message`.
    /// @param message The message to write; its type must be one of the family.
    /// @param writer The destination.
    /// @return False if the writer became invalid (the message did not fit) or a field broke its limit.
    template <typename M>
    static bool encode(const M& message, ByteWriter& writer) noexcept;

    /// @brief Encodes `message` into a buffer of exactly the right size.
    /// @param message The message to write; its type must be one of the family.
    /// @return The bytes, or an empty buffer if a field breaks its limit.
    template <typename M>
    [[nodiscard]] static Bytes encode(const M& message);

    /// @brief Reads the kind byte without decoding the rest.
    /// @param bytes The datagram.
    /// @return The kind, or nullopt if `bytes` is empty or the first byte is not a kind of the family.
    [[nodiscard]] static std::optional<Kind> peekKind(std::span<const std::byte> bytes) noexcept;

    /// @brief Decodes a whole datagram.
    /// @param bytes The datagram; a decoded message may point into it, so it must outlive the message.
    /// @return The message, or nullopt on an empty input, an unknown kind, a failed check, missing bytes or bytes
    /// left over.
    [[nodiscard]] static std::optional<Message> decode(std::span<const std::byte> bytes) noexcept;

  private:
    using DecodeFunction = std::optional<Message> (*)(ByteReader&) noexcept;

    /// @brief Reads the fields of an `M` after its kind byte, and checks that nothing is left.
    template <typename M>
    [[nodiscard]] static std::optional<Message> decodeAs(ByteReader& reader) noexcept;

    /// @brief Builds the table from kind byte to decoder; nullptr for a byte that is not a kind.
    [[nodiscard]] static constexpr std::array<DecodeFunction, 256> makeTable() noexcept;
};
}  // namespace rtype::network::bytes

#include "Codec.tpp"  // IWYU pragma: export
