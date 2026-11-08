/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketCodec
*/

#include "rtype/replication/packet/PacketCodec.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/MessageType.hpp"

namespace rtype::replication::packet {
namespace {
/// @return The range of an integer type, as doubles.
struct Range {
    double low;
    double high;
};

Range rangeOf(const PacketType type) noexcept {
    switch (type) {
        case PacketType::kU8:
            return {0.0, 255.0};
        case PacketType::kU16:
            return {0.0, 65535.0};
        case PacketType::kU32:
            return {0.0, 4294967295.0};
        case PacketType::kI32:
            return {-2147483648.0, 2147483647.0};
        default:
            return {0.0, 0.0};
    }
}

/// @brief Checks one value against its field.
/// @return Why it does not fit, empty if it does.
std::string checkValue(const PacketField& field, const PacketValue& value) {
    if (field.type == PacketType::kString) {
        const auto* text = std::get_if<std::string>(&value);
        if (text == nullptr) {
            return std::format("field '{}' must be a string", field.name);
        }
        return text->size() > field.maxLength
                   ? std::format("field '{}' is {} bytes, at most {}", field.name, text->size(), field.maxLength)
                   : std::string{};
    }
    if (field.type == PacketType::kBool) {
        return std::holds_alternative<bool>(value) ? std::string{}
                                                   : std::format("field '{}' must be a boolean", field.name);
    }
    const auto* number = std::get_if<double>(&value);
    if (number == nullptr || !std::isfinite(*number)) {
        return std::format("field '{}' must be a finite number", field.name);
    }
    if (field.type == PacketType::kF32) {
        return std::fabs(*number) <= static_cast<double>(std::numeric_limits<float>::max())
                   ? std::string{}
                   : std::format("field '{}' does not fit a float", field.name);
    }
    const Range range = rangeOf(field.type);
    if (*number != std::floor(*number) || *number < range.low || *number > range.high) {
        return std::format("field '{}' must be a whole number from {} to {}", field.name, range.low, range.high);
    }
    return {};
}

/// @brief Reads a decimal number, all of @p text.
std::optional<double> parseNumber(const std::string_view text) {
    double value = 0.0;
    const auto* end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}
}  // namespace

std::optional<PacketValues> parseValues(const PacketSchema& schema, const std::string_view text, std::string& error) {
    PacketValues values;
    std::size_t position = 0;
    for (const PacketField& field : schema.fields) {
        const std::size_t colon = text.find(':', position);
        std::size_t size = 0;
        if (colon == std::string_view::npos ||
            std::from_chars(text.data() + position, text.data() + colon, size).ptr != text.data() + colon) {
            error = std::format("field '{}' is missing", field.name);
            return std::nullopt;
        }
        if (size > text.size() - colon - 1) {
            error = std::format("field '{}' is cut short", field.name);
            return std::nullopt;
        }
        const std::string_view raw = text.substr(colon + 1, size);
        position = colon + 1 + size;
        PacketValue value;
        if (field.type == PacketType::kString) {
            value = std::string{raw};
        } else if (field.type == PacketType::kBool) {
            if (raw != "0" && raw != "1") {
                error = std::format("field '{}' must be a boolean", field.name);
                return std::nullopt;
            }
            value = raw == "1";
        } else {
            const std::optional<double> number = parseNumber(raw);
            if (!number) {
                error = std::format("field '{}' must be a number", field.name);
                return std::nullopt;
            }
            value = *number;
        }
        error = checkValue(field, value);
        if (!error.empty()) {
            return std::nullopt;
        }
        values.push_back(std::move(value));
    }
    if (position != text.size()) {
        error = "unexpected extra data after the last field";
        return std::nullopt;
    }
    return values;
}

std::string formatValues(const PacketSchema& schema, const PacketValues& values) {
    std::string text;
    for (std::size_t index = 0; index < schema.fields.size() && index < values.size(); ++index) {
        std::string field;
        if (const auto* string = std::get_if<std::string>(&values[index])) {
            field = *string;
        } else if (const auto* flag = std::get_if<bool>(&values[index])) {
            field = *flag ? "1" : "0";
        } else {
            field = std::format("{}", std::get<double>(values[index]));
        }
        text += std::format("{}:{}", field.size(), field);
    }
    return text;
}

network::Bytes encodePacket(const PacketSchema& schema, const PacketValues& values) {
    network::Bytes bytes(schema.maxMessageSize());
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kCustomPacket));
    writer.writeU8(static_cast<std::uint8_t>(schema.name.size()));
    writer.writeBytes(std::as_bytes(std::span{schema.name}));
    writer.writeU32(schema.hash());
    for (std::size_t index = 0; index < schema.fields.size() && index < values.size(); ++index) {
        const PacketValue& value = values[index];
        switch (schema.fields[index].type) {
            case PacketType::kBool:
                writer.writeU8(std::get<bool>(value) ? 1U : 0U);
                break;
            case PacketType::kU8:
                writer.writeU8(static_cast<std::uint8_t>(std::get<double>(value)));
                break;
            case PacketType::kU16:
                writer.writeU16(static_cast<std::uint16_t>(std::get<double>(value)));
                break;
            case PacketType::kU32:
                writer.writeU32(static_cast<std::uint32_t>(std::get<double>(value)));
                break;
            case PacketType::kI32:
                writer.writeI32(static_cast<std::int32_t>(std::get<double>(value)));
                break;
            case PacketType::kF32:
                writer.writeF32(static_cast<float>(std::get<double>(value)));
                break;
            case PacketType::kString: {
                const std::string& text = std::get<std::string>(value);
                writer.writeU16(static_cast<std::uint16_t>(text.size()));
                writer.writeBytes(std::as_bytes(std::span{text}));
                break;
            }
        }
    }
    bytes.resize(writer.bytesWritten());
    return bytes;
}

std::optional<DecodedPacket> decodePacket(const PacketRegistry& registry, const PacketDirection direction,
                                          const std::span<const std::byte> payload) {
    network::bytes::ByteReader reader{payload};
    if (reader.readU8() != static_cast<std::uint8_t>(MessageType::kCustomPacket)) {
        return std::nullopt;
    }
    const std::uint8_t nameSize = reader.readU8();
    const std::span<const std::byte> nameBytes = reader.readBytes(nameSize);
    const std::uint32_t hash = reader.readU32();
    if (!reader.isValid()) {
        return std::nullopt;
    }
    const std::string_view name{reinterpret_cast<const char*>(nameBytes.data()), nameBytes.size()};
    const PacketSchema* schema = registry.find(name);
    if (schema == nullptr || schema->direction != direction || schema->hash() != hash) {
        return std::nullopt;
    }
    DecodedPacket decoded{.name = std::string{name}, .values = {}};
    for (const PacketField& field : schema->fields) {
        PacketValue value;
        switch (field.type) {
            case PacketType::kBool: {
                const std::uint8_t flag = reader.readU8();
                if (flag > 1) {
                    return std::nullopt;
                }
                value = flag == 1;
                break;
            }
            case PacketType::kU8:
                value = static_cast<double>(reader.readU8());
                break;
            case PacketType::kU16:
                value = static_cast<double>(reader.readU16());
                break;
            case PacketType::kU32:
                value = static_cast<double>(reader.readU32());
                break;
            case PacketType::kI32:
                value = static_cast<double>(reader.readI32());
                break;
            case PacketType::kF32:
                value = static_cast<double>(reader.readF32());
                break;
            case PacketType::kString: {
                const std::uint16_t size = reader.readU16();
                if (size > field.maxLength) {
                    return std::nullopt;
                }
                const std::span<const std::byte> text = reader.readBytes(size);
                value = std::string{reinterpret_cast<const char*>(text.data()), text.size()};
                break;
            }
        }
        if (!reader.isValid() || !checkValue(field, value).empty()) {
            return std::nullopt;
        }
        decoded.values.push_back(std::move(value));
    }
    if (!reader.isValid() || reader.remainingSize() != 0) {
        return std::nullopt;
    }
    return decoded;
}
}  // namespace rtype::replication::packet
