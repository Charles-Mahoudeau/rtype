/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Codec
*/

#pragma once

namespace rtype::network::bytes {
template <typename Kind, typename... Messages>
template <typename M>
bool Codec<Kind, std::variant<Messages...>>::encode(const M& message, ByteWriter& writer) noexcept {
    static_assert((std::is_same_v<M, Messages> || ...), "Codec::encode: the message is not part of the family");
    writer.writeU8(static_cast<std::uint8_t>(M::kKind));
    WriteStream stream{writer};
    M::serialize(message, stream);
    return stream.isValid();
}

template <typename Kind, typename... Messages>
template <typename M>
Bytes Codec<Kind, std::variant<Messages...>>::encode(const M& message) {
    static_assert((std::is_same_v<M, Messages> || ...), "Codec::encode: the message is not part of the family");
    SizeStream counter{1};
    M::serialize(message, counter);
    if (!counter.isValid()) {
        return {};
    }
    Bytes bytes(counter.size());
    if (ByteWriter writer{bytes}; !encode(message, writer)) {
        return {};
    }
    return bytes;
}

template <typename Kind, typename... Messages>
std::optional<Kind> Codec<Kind, std::variant<Messages...>>::peekKind(const std::span<const std::byte> bytes) noexcept {
    static constexpr std::array<DecodeFunction, 256> kTable = makeTable();
    if (bytes.empty() || kTable[std::to_integer<std::uint8_t>(bytes.front())] == nullptr) {
        return std::nullopt;
    }
    return static_cast<Kind>(std::to_integer<std::uint8_t>(bytes.front()));
}

template <typename Kind, typename... Messages>
std::optional<std::variant<Messages...>> Codec<Kind, std::variant<Messages...>>::decode(
    const std::span<const std::byte> bytes) noexcept {
    static constexpr std::array<DecodeFunction, 256> kTable = makeTable();
    ByteReader reader{bytes};
    const DecodeFunction decoder = kTable[reader.readU8()];
    if (!reader.isValid() || decoder == nullptr) {
        return std::nullopt;
    }
    return decoder(reader);
}

template <typename Kind, typename... Messages>
template <typename M>
std::optional<std::variant<Messages...>> Codec<Kind, std::variant<Messages...>>::decodeAs(ByteReader& reader) noexcept {
    M message{};
    ReadStream stream{reader, 1};
    M::serialize(message, stream);
    if (!stream.isValid() || !reader.isFinished()) {
        return std::nullopt;
    }
    return Message{std::move(message)};
}

template <typename Kind, typename... Messages>
constexpr std::array<typename Codec<Kind, std::variant<Messages...>>::DecodeFunction, 256>
Codec<Kind, std::variant<Messages...>>::makeTable() noexcept {
    std::array<DecodeFunction, 256> table{};
    ((table[static_cast<std::uint8_t>(Messages::kKind)] = &decodeAs<Messages>), ...);
    return table;
}
}  // namespace rtype::network::bytes
