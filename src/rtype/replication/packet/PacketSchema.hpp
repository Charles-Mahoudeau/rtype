/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketSchema
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::replication::packet {
inline constexpr std::size_t kMaxPacketNameSize = 64;  ///< The longest packet name, in bytes.
inline constexpr std::size_t kMaxFields = 32;          ///< The most fields a packet has.
inline constexpr std::size_t kMaxStringSize = 1000;    ///< The longest `maxLength` a string field can ask for.
inline constexpr std::size_t kDefaultStringSize = 64;  ///< `maxLength` of a string field that does not say.
inline constexpr std::size_t kMaxMessageSize = 1000;   ///< The most bytes of one message: the transport's limit.

/// @brief What a field holds.
enum class PacketType : std::uint8_t {
    kBool,    ///< One byte, 0 or 1.
    kU8,      ///< 0 to 255.
    kU16,     ///< 0 to 65 535.
    kU32,     ///< 0 to 4 294 967 295.
    kI32,     ///< -2^31 to 2^31 - 1.
    kF32,     ///< A finite float.
    kString,  ///< Bytes (text), at most `maxLength` of them; a `u16` length precedes them on the wire.
};

/// @brief Who may send a packet.
enum class PacketDirection : std::uint8_t {
    kToServer,   ///< Client to server: the server validates it.
    kToClients,  ///< Server to client.
};

/// @brief One field of a packet.
struct PacketField {
    std::string name;                  ///< The field's name: a Luau identifier, unique in the packet.
    PacketType type{PacketType::kU8};  ///< What it holds.
    std::uint16_t maxLength{0};        ///< For a string, its longest size in bytes; 0 for the other types.
};

/// @brief The description of a packet, the same on both sides: name, direction, channel and ordered fields.
struct PacketSchema {
    std::string name;                                       ///< `set_directive`, `game.Buy`...
    PacketDirection direction{PacketDirection::kToServer};  ///< Who sends it.
    network::Channel channel{network::Channel::kReliable};  ///< How it travels.
    std::vector<PacketField> fields;                        ///< In wire order.

    /// @return FNV-1a 32 of the name, direction, channel and every field: a message carries it, and a message whose
    /// hash differs from the receiver's (its scripts declare the packet otherwise) is dropped.
    [[nodiscard]] std::uint32_t hash() const noexcept;

    /// @return The size of the largest message of this packet, header included.
    [[nodiscard]] std::size_t maxMessageSize() const noexcept;
};

/// @brief A value of a field: a bool, a number (an integer or a float, exactly as a Luau number) or a string.
using PacketValue = std::variant<bool, double, std::string>;

/// @brief The values of a packet, in the order of its fields.
using PacketValues = std::vector<PacketValue>;

/// @brief The packets a game declares, by name. An ECS resource named "net.PacketRegistry".
///
/// @details Scripts declare packets when they load (`engine.net.packet`); the server and the client plugins freeze the
/// registry when they start, so a packet is never declared once messages travel.
class RTYPE_REPLICATION_API PacketRegistry {
  public:
    /// @brief Declares a packet.
    /// @throws exceptions::PacketException When the registry is frozen, the name is taken or invalid, a field is
    /// invalid (name, type, length) or duplicated, there are too many fields, or the largest message would not fit in
    /// kMaxMessageSize.
    void declare(PacketSchema schema);

    /// @brief Stops further declarations.
    void freeze() noexcept;

    /// @return Whether freeze() was called.
    [[nodiscard]] bool isFrozen() const noexcept;

    /// @return The packet of this name, or null.
    [[nodiscard]] const PacketSchema* find(std::string_view name) const noexcept;

    /// @return How many packets are declared.
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    std::vector<PacketSchema> _schemas;  ///< In declaration order (few: a linear search; moves never throw).
    bool _frozen{false};                 ///< Set by freeze().
};

RTYPE_ECS_RESOURCE(PacketRegistry, "net.PacketRegistry");
}  // namespace rtype::replication::packet
