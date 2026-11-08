/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketSchema
*/

#include "rtype/replication/packet/PacketSchema.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "rtype/replication/exceptions/ReplicationExceptions.hpp"

namespace rtype::replication::packet {
namespace {
constexpr std::uint32_t kFnvOffset = 2166136261U;
constexpr std::uint32_t kFnvPrime = 16777619U;
constexpr std::size_t kMessageOverhead = 1 + 1 + 4;  // type byte, name length, schema hash

void mix(std::uint32_t& hash, const std::uint8_t byte) noexcept { hash = (hash ^ byte) * kFnvPrime; }

void mix(std::uint32_t& hash, const std::string_view text) noexcept {
    for (const char character : text) {
        mix(hash, static_cast<std::uint8_t>(character));
    }
    mix(hash, std::uint8_t{0});
}

/// @return The bytes a field takes on the wire, at most.
std::size_t wireSize(const PacketField& field) noexcept {
    switch (field.type) {
        case PacketType::kBool:
        case PacketType::kU8:
            return 1;
        case PacketType::kU16:
            return 2;
        case PacketType::kU32:
        case PacketType::kI32:
        case PacketType::kF32:
            return 4;
        case PacketType::kString:
            return 2 + static_cast<std::size_t>(field.maxLength);
    }
    return 0;
}

/// @return Whether @p name is a Luau identifier: letters, digits and underscores, not starting with a digit.
bool isIdentifier(const std::string_view name) noexcept {
    return !name.empty() && std::isdigit(static_cast<unsigned char>(name.front())) == 0 &&
           std::ranges::all_of(name, [](const char character) {
               return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_';
           });
}

/// @return Whether @p name is a valid packet name: identifiers separated by dots (`game.Buy`).
bool isPacketName(const std::string_view name) noexcept {
    if (name.empty() || name.size() > kMaxPacketNameSize) {
        return false;
    }
    std::size_t start = 0;
    while (start <= name.size()) {
        const std::size_t end = std::min(name.find('.', start), name.size());
        if (!isIdentifier(name.substr(start, end - start))) {
            return false;
        }
        start = end + 1;
    }
    return true;
}
}  // namespace

std::uint32_t PacketSchema::hash() const noexcept {
    std::uint32_t result = kFnvOffset;
    mix(result, name);
    mix(result, static_cast<std::uint8_t>(direction));
    mix(result, static_cast<std::uint8_t>(channel));
    for (const PacketField& field : fields) {
        mix(result, field.name);
        mix(result, static_cast<std::uint8_t>(field.type));
        mix(result, static_cast<std::uint8_t>(field.maxLength & 0xFFU));
        mix(result, static_cast<std::uint8_t>(field.maxLength >> 8U));
    }
    return result;
}

std::size_t PacketSchema::maxMessageSize() const noexcept {
    std::size_t size = kMessageOverhead + name.size();
    for (const PacketField& field : fields) {
        size += wireSize(field);
    }
    return size;
}

void PacketRegistry::declare(PacketSchema schema) {
    if (_frozen) {
        throw exceptions::PacketException{
            std::format("Cannot declare packet '{}': packets are declared while the scripts load, not after the "
                        "network started",
                        schema.name)};
    }
    if (!isPacketName(schema.name)) {
        throw exceptions::PacketException{
            std::format("Invalid packet name '{}': expected identifiers separated by dots, at most {} bytes",
                        schema.name, kMaxPacketNameSize)};
    }
    if (_schemas.contains(schema.name)) {
        throw exceptions::PacketException{std::format("Packet '{}' is already declared", schema.name)};
    }
    if (schema.fields.size() > kMaxFields) {
        throw exceptions::PacketException{
            std::format("Packet '{}' has {} fields, at most {}", schema.name, schema.fields.size(), kMaxFields)};
    }
    std::unordered_set<std::string_view> seen;
    for (PacketField& field : schema.fields) {
        if (!isIdentifier(field.name) || field.name == "sender") {
            throw exceptions::PacketException{
                std::format("Packet '{}': invalid field name '{}'", schema.name, field.name)};
        }
        if (!seen.insert(field.name).second) {
            throw exceptions::PacketException{
                std::format("Packet '{}': field '{}' is declared twice", schema.name, field.name)};
        }
        if (field.type != PacketType::kString) {
            field.maxLength = 0;
        } else if (field.maxLength == 0) {
            field.maxLength = static_cast<std::uint16_t>(kDefaultStringSize);
        } else if (field.maxLength > kMaxStringSize) {
            throw exceptions::PacketException{std::format("Packet '{}': string '{}' asks for {} bytes, at most {}",
                                                          schema.name, field.name, field.maxLength, kMaxStringSize)};
        }
    }
    if (schema.maxMessageSize() > kMaxMessageSize) {
        throw exceptions::PacketException{
            std::format("Packet '{}' could take {} bytes, more than the {} of a message: shorten its strings",
                        schema.name, schema.maxMessageSize(), kMaxMessageSize)};
    }
    const std::string name = schema.name;
    _schemas.emplace(name, std::move(schema));
}

void PacketRegistry::freeze() noexcept { _frozen = true; }

bool PacketRegistry::isFrozen() const noexcept { return _frozen; }

const PacketSchema* PacketRegistry::find(const std::string_view name) const noexcept {
    const auto found = _schemas.find(name);
    return found == _schemas.end() ? nullptr : &found->second;
}

std::size_t PacketRegistry::size() const noexcept { return _schemas.size(); }
}  // namespace rtype::replication::packet
