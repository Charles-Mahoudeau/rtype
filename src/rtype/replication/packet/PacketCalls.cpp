/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketCalls
*/

#include "rtype/replication/packet/PacketCalls.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/packet/PacketCodec.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"
#include "rtype/replication/server/ServerState.hpp"
#include "rtype/replication/standalone/LocalSession.hpp"

namespace rtype::replication::packet {
namespace {
using exceptions::PacketException;

PacketType typeNamed(const std::string_view packet, const std::string_view name) {
    static constexpr std::array<std::pair<std::string_view, PacketType>, 7> kTypes{
        {
            {"bool", PacketType::kBool},
            {"u8", PacketType::kU8},
            {"u16", PacketType::kU16},
            {"u32", PacketType::kU32},
            {"i32", PacketType::kI32},
            {"f32", PacketType::kF32},
            {"string", PacketType::kString},
        },
    };
    for (const auto& [text, type] : kTypes) {
        if (text == name) {
            return type;
        }
    }
    throw PacketException{std::format("Packet '{}': unknown field type '{}'", packet, name)};
}

/// @brief Reads `name:type[:maxLength]` lines.
std::vector<PacketField> parseFields(const std::string_view packet, const std::string_view lines) {
    std::vector<PacketField> fields;
    std::size_t start = 0;
    while (start < lines.size()) {
        const std::size_t end = std::min(lines.find('\n', start), lines.size());
        const std::string_view line = lines.substr(start, end - start);
        start = end + 1;
        const std::size_t first = line.find(':');
        if (first == std::string_view::npos) {
            throw PacketException{std::format("Packet '{}': invalid field '{}'", packet, line)};
        }
        const std::size_t second = line.find(':', first + 1);
        PacketField field{
            .name = std::string{line.substr(0, first)},
            .type = typeNamed(packet, line.substr(first + 1, second == std::string_view::npos ? std::string_view::npos
                                                                                              : second - first - 1)),
            .maxLength = 0,
        };
        if (second != std::string_view::npos) {
            const std::string length{line.substr(second + 1)};
            const bool digits = !length.empty() && length.size() <= 4 &&
                                std::ranges::all_of(length, [](const char c) { return c >= '0' && c <= '9'; });
            const unsigned long value = digits ? std::stoul(length) : kMaxStringSize + 1;
            if (value > kMaxStringSize) {
                throw PacketException{std::format("Packet '{}': invalid length in '{}'", packet, line)};
            }
            field.maxLength = static_cast<std::uint16_t>(value);
        }
        fields.push_back(std::move(field));
    }
    return fields;
}

void declare(ecs::ResMut<PacketRegistry> registry, const DeclareArguments& arguments) {
    PacketSchema schema{.name = arguments.name, .direction = {}, .channel = {}, .fields = {}};
    if (arguments.direction == "toServer") {
        schema.direction = PacketDirection::kToServer;
    } else if (arguments.direction == "toClients") {
        schema.direction = PacketDirection::kToClients;
    } else {
        throw PacketException{std::format(R"(Packet '{}': direction must be "toServer" or "toClients", got '{}')",
                                          arguments.name, arguments.direction)};
    }
    if (arguments.channel == "reliable") {
        schema.channel = network::Channel::kReliable;
    } else if (arguments.channel == "unreliable") {
        schema.channel = network::Channel::kUnreliable;
    } else {
        throw PacketException{std::format(R"(Packet '{}': channel must be "reliable" or "unreliable", got '{}')",
                                          arguments.name, arguments.channel)};
    }
    schema.fields = parseFields(arguments.name, arguments.fields);
    registry->declare(std::move(schema));
}

const char* typeText(const PacketType type) noexcept {
    switch (type) {
        case PacketType::kBool:
            return "bool";
        case PacketType::kU8:
            return "u8";
        case PacketType::kU16:
            return "u16";
        case PacketType::kU32:
            return "u32";
        case PacketType::kI32:
            return "i32";
        case PacketType::kF32:
            return "f32";
        case PacketType::kString:
            return "string";
    }
    return "string";
}

ListResult list(ecs::Res<PacketRegistry> registry, const ListArguments& arguments) {
    ListResult result;
    for (const PacketSchema& schema : registry->all()) {
        if (!schema.name.starts_with(arguments.prefix)) {
            continue;
        }
        result.packets += schema.name;
        for (const PacketField& field : schema.fields) {
            result.packets += std::format(" {}:{}", field.name, typeText(field.type));
        }
        result.packets += '\n';
    }
    return result;
}

/// @brief Finds a packet that goes @p direction and writes its message from the script's values.
network::Bytes makeMessage(const PacketRegistry& registry, const std::string& name, const PacketDirection direction,
                           const std::string& values, network::Channel& channel) {
    const PacketSchema* schema = registry.find(name);
    if (schema == nullptr) {
        throw PacketException{std::format("Unknown packet '{}'", name)};
    }
    if (schema->direction != direction) {
        throw PacketException{
            std::format("Packet '{}' goes {}: this side cannot send it", name,
                        schema->direction == PacketDirection::kToServer ? "to the server" : "to the clients")};
    }
    std::string error;
    const std::optional<PacketValues> parsed = parseValues(*schema, values, error);
    if (!parsed) {
        throw PacketException{std::format("Packet '{}': {}", name, error)};
    }
    channel = schema->channel;
    return encodePacket(*schema, *parsed);
}

SendResult sendToServer(ecs::ResMut<client::ClientState> state, ecs::Res<PacketRegistry> registry,
                        const SendToServerArguments& arguments) {
    network::Channel channel{};
    const network::Bytes message =
        makeMessage(*registry, arguments.name, PacketDirection::kToServer, arguments.values, channel);
    return SendResult{.sent = state->sendPacket(channel, message)};
}

SendResult sendToClients(ecs::ResMut<server::ServerState> state, ecs::Res<PacketRegistry> registry,
                         const SendToClientsArguments& arguments) {
    network::Channel channel{};
    network::Bytes message =
        makeMessage(*registry, arguments.name, PacketDirection::kToClients, arguments.values, channel);
    return SendResult{.sent = state->sendPacket(arguments.client, channel, std::move(message))};
}

/// @brief Standalone: the packet waits in the session for the next PreUpdate, which delivers it to the server side.
SendResult sendToServerLocally(ecs::ResMut<standalone::LocalSession> session, ecs::Res<PacketRegistry> registry,
                               const SendToServerArguments& arguments) {
    network::Channel channel{};
    session->queuePacket(PacketDirection::kToServer,
                         makeMessage(*registry, arguments.name, PacketDirection::kToServer, arguments.values, channel));
    return SendResult{.sent = true};
}

/// @brief Standalone: as above toward the client side; the only client is client 1.
SendResult sendToClientsLocally(ecs::ResMut<standalone::LocalSession> session, ecs::Res<PacketRegistry> registry,
                                const SendToClientsArguments& arguments) {
    network::Channel channel{};
    network::Bytes message =
        makeMessage(*registry, arguments.name, PacketDirection::kToClients, arguments.values, channel);
    if (arguments.client != 0 && arguments.client != standalone::LocalSession::kClient) {
        return SendResult{.sent = false};
    }
    session->queuePacket(PacketDirection::kToClients, std::move(message));
    return SendResult{.sent = true};
}
}  // namespace

void addPacketCalls(ecs::App& app) {
    static constexpr auto kForms = std::to_array<char>({
#include "NetForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    app.getPluginRegistry().addScriptForms("addNetForms", std::string_view{kForms.data(), kForms.size() - 1});
    app.addCall(std::string{kDeclareCall}, &declare);
    app.addCall(std::string{kListCall}, &list);
    const ecs::RunMode mode = app.getSchedules().getRunMode();
    if (app.getWorld().getResources().has<standalone::LocalSession>()) {
        app.addCall(std::string{kSendToServerCall}, &sendToServerLocally);
        app.addCall(std::string{kSendToClientsCall}, &sendToClientsLocally);
        return;
    }
    if (ecs::runsIn(ecs::Side::kClient, mode)) {
        app.addCall(std::string{kSendToServerCall}, &sendToServer);
    }
    if (ecs::runsIn(ecs::Side::kServer, mode)) {
        app.addCall(std::string{kSendToClientsCall}, &sendToClients);
    }
}
}  // namespace rtype::replication::packet
