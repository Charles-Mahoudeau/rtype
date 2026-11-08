/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketCodec
*/

#include "rtype/replication/packet/PacketCodec.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <locale>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"

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
            return Range{.low = 0.0, .high = 255.0};
        case PacketType::kU16:
            return Range{.low = 0.0, .high = 65535.0};
        case PacketType::kU32:
            return Range{.low = 0.0, .high = 4294967295.0};
        case PacketType::kI32:
            return Range{.low = -2147483648.0, .high = 2147483647.0};
        default:
            return Range{.low = 0.0, .high = 0.0};
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
    if (text.empty() || std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        return std::nullopt;
    }
    // A stream imbued with the classic locale reads '.' whatever the program's numeric locale is (stod does not).
    std::istringstream stream{std::string{text}};
    stream.imbue(std::locale::classic());
    double value = 0;
    stream >> value;
    // Fails on text that is not a number, or out of the range of a double.
    return !stream.fail() && stream.peek() == std::istringstream::traits_type::eof() ? std::optional<double>{value}
                                                                                     : std::nullopt;
}

/// @brief Reads a size written in decimal digits, all of @p text.
std::optional<std::size_t> parseSize(const std::string_view text) {
    constexpr std::size_t kMaxDigits = 9;
    if (text.empty() || text.size() > kMaxDigits ||
        !std::ranges::all_of(text, [](const char c) { return c >= '0' && c <= '9'; })) {
        return std::nullopt;
    }
    std::size_t value = 0;
    for (const char digit : text) {
        value = (value * 10) + static_cast<std::size_t>(digit - '0');
    }
    return value;
}

/// @return The bytes as text.
std::string asText(const std::span<const std::byte> bytes) {
    std::string text(bytes.size(), '\0');
    std::ranges::transform(bytes, text.begin(), [](const std::byte byte) { return static_cast<char>(byte); });
    return text;
}
}  // namespace

std::optional<PacketValues> parseValues(const PacketSchema& schema, const std::string_view text, std::string& error) {
    PacketValues values;
    std::size_t position = 0;
    for (const PacketField& field : schema.fields) {
        const std::size_t colon = text.find(':', position);
        const std::optional<std::size_t> size =
            colon == std::string_view::npos ? std::nullopt : parseSize(text.substr(position, colon - position));
        if (!size) {
            error = std::format("field '{}' is missing", field.name);
            return std::nullopt;
        }
        if (*size > text.size() - colon - 1) {
            error = std::format("field '{}' is cut short", field.name);
            return std::nullopt;
        }
        const std::string_view raw = text.substr(colon + 1, *size);
        position = colon + 1 + *size;
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
        if (const auto* string = std::get_if<std::string>(&values.at(index))) {
            field = *string;
        } else if (const auto* flag = std::get_if<bool>(&values.at(index))) {
            field = *flag ? "1" : "0";
        } else {
            field = std::format("{}", std::get<double>(values.at(index)));
        }
        text += std::format("{}:{}", field.size(), field);
    }
    return text;
}

network::Bytes encodePacket(const PacketSchema& schema, const PacketValues& values) {
    auto bytes = network::Bytes(schema.maxMessageSize());
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kCustomPacket));
    writer.writeU8(static_cast<std::uint8_t>(schema.name.size()));
    writer.writeBytes(std::as_bytes(std::span{schema.name}));
    writer.writeU32(schema.hash());
    for (std::size_t index = 0; index < schema.fields.size() && index < values.size(); ++index) {
        const PacketValue& value = values.at(index);
        switch (schema.fields.at(index).type) {
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
                const auto& text = std::get<std::string>(value);
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
    const std::string name = asText(nameBytes);
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
                value = std::string{asText(text)};
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
