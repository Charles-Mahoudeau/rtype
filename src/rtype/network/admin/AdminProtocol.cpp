/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminProtocol
*/

#include "rtype/network/admin/AdminProtocol.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/server/Session.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace rtype::network::admin {
namespace {
constexpr std::size_t kHeaderSize = 5;                           // type, id
constexpr std::size_t kClientsPrefixSize = kHeaderSize + 4 + 2;  // total, count
constexpr std::size_t kClientEntrySize = 4 + 1 + 16 + 2 + 4 + 8 + 8;

constexpr std::uint8_t toByte(const MessageType type) noexcept { return static_cast<std::uint8_t>(type); }

constexpr bool isExtensionType(const std::uint8_t type) noexcept {
    return type >= kWorldFirst && type <= kExtensionLast;
}

void writeSettings(bytes::ByteWriter& writer, const simulation::SimulationSettings& settings) {
    writer.writeU16(settings.latencyMs);
    writer.writeU16(settings.jitterMs);
    writer.writeF32(settings.lossPercent);
}

simulation::SimulationSettings readSettings(bytes::ByteReader& reader) {
    simulation::SimulationSettings settings;
    settings.latencyMs = reader.readU16();
    settings.jitterMs = reader.readU16();
    settings.lossPercent = reader.readF32();
    return settings;
}

void writeEntry(bytes::ByteWriter& writer, const ClientEntry& entry) {
    writer.writeU32(entry.client);
    writer.writeU8(static_cast<std::uint8_t>(entry.state));
    writer.writeBytes(std::as_bytes(std::span{entry.address.bytes()}));
    writer.writeU16(entry.address.port());
    writer.writeF32(entry.rttMs);
    writer.writeU64(entry.droppedMessages);
    writer.writeU64(entry.protocolErrors);
}

std::optional<ClientEntry> readEntry(bytes::ByteReader& reader) {
    ClientEntry entry;
    entry.client = reader.readU32();
    const std::uint8_t state = reader.readU8();
    const auto raw = reader.readBytes(16);
    const std::uint16_t port = reader.readU16();
    entry.rttMs = reader.readF32();
    entry.droppedMessages = reader.readU64();
    entry.protocolErrors = reader.readU64();
    if (!reader.isValid() || state > static_cast<std::uint8_t>(server::SessionState::kReady)) {
        return std::nullopt;
    }
    entry.state = static_cast<server::SessionState>(state);
    io::Address::Bytes16 bytes16{};
    std::ranges::transform(raw, bytes16.begin(), [](const std::byte b) { return static_cast<std::uint8_t>(b); });
    entry.address = io::Address{bytes16, port};
    return entry;
}

/// @return The type byte of @p reply, with the reply flag.
std::uint8_t replyType(const Reply& reply) {
    return std::visit(
        [](const auto& value) -> std::uint8_t {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, HelloReply>) {
                return toByte(MessageType::kHello) | kReplyFlag;
            } else if constexpr (std::is_same_v<T, StatusReply>) {
                return toByte(MessageType::kStatus) | kReplyFlag;
            } else if constexpr (std::is_same_v<T, ClientsReply>) {
                return toByte(MessageType::kClients) | kReplyFlag;
            } else if constexpr (std::is_same_v<T, KickReply>) {
                return toByte(MessageType::kKick) | kReplyFlag;
            } else if constexpr (std::is_same_v<T, SimulationReply>) {
                return toByte(MessageType::kSimulationGet) | kReplyFlag;
            } else if constexpr (std::is_same_v<T, ErrorReply>) {
                return toByte(MessageType::kError);
            } else {
                return value.type;
            }
        },
        reply);
}
}  // namespace

std::size_t maxClientEntries() noexcept { return (protocol::kMaxPacketSize - kClientsPrefixSize) / kClientEntrySize; }

Bytes encodeRequest(const std::uint32_t id, const std::string& token, const Request& request) {
    Bytes buffer(protocol::kMaxPacketSize);
    bytes::ByteWriter writer{buffer};
    const std::uint8_t type = std::visit(
        [](const auto& value) -> std::uint8_t {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, HelloRequest>) {
                return toByte(MessageType::kHello);
            } else if constexpr (std::is_same_v<T, StatusRequest>) {
                return toByte(MessageType::kStatus);
            } else if constexpr (std::is_same_v<T, ClientsRequest>) {
                return toByte(MessageType::kClients);
            } else if constexpr (std::is_same_v<T, KickRequest>) {
                return toByte(MessageType::kKick);
            } else if constexpr (std::is_same_v<T, SimulationGetRequest>) {
                return toByte(MessageType::kSimulationGet);
            } else if constexpr (std::is_same_v<T, SimulationSetRequest>) {
                return toByte(MessageType::kSimulationSet);
            } else {
                return value.type;
            }
        },
        request);
    writer.writeU8(type);
    writer.writeU32(id);
    writer.writeString(token);
    if (const auto* kick = std::get_if<KickRequest>(&request)) {
        writer.writeU32(kick->client);
    } else if (const auto* simulation = std::get_if<SimulationSetRequest>(&request)) {
        writeSettings(writer, simulation->settings);
    } else if (const auto* extension = std::get_if<ExtensionRequest>(&request)) {
        writer.writeBytes(extension->body);
    }
    if (!writer.isValid()) {
        return {};
    }
    const auto written = writer.writtenBytes();
    return {written.begin(), written.end()};
}

std::optional<RequestFrame> decodeRequest(const std::span<const std::byte> datagram) {
    bytes::ByteReader reader{datagram};
    RequestFrame frame;
    frame.type = reader.readU8();
    frame.id = reader.readU32();
    frame.token = reader.readString();
    if (!reader.isValid()) {
        return std::nullopt;
    }
    const auto expectEnd = [&reader, &frame] {
        if (!reader.isValid() || reader.remainingSize() != 0) {
            frame.request.reset();
            frame.problem = ErrorCode::kMalformed;
            return false;
        }
        return true;
    };
    switch (frame.type) {
        case static_cast<std::uint8_t>(MessageType::kHello):
            frame.request = HelloRequest{};
            static_cast<void>(expectEnd());
            break;
        case static_cast<std::uint8_t>(MessageType::kStatus):
            frame.request = StatusRequest{};
            static_cast<void>(expectEnd());
            break;
        case static_cast<std::uint8_t>(MessageType::kClients):
            frame.request = ClientsRequest{};
            static_cast<void>(expectEnd());
            break;
        case static_cast<std::uint8_t>(MessageType::kKick):
            frame.request = KickRequest{.client = reader.readU32()};
            static_cast<void>(expectEnd());
            break;
        case static_cast<std::uint8_t>(MessageType::kSimulationGet):
            frame.request = SimulationGetRequest{};
            static_cast<void>(expectEnd());
            break;
        case static_cast<std::uint8_t>(MessageType::kSimulationSet):
            frame.request = SimulationSetRequest{.settings = readSettings(reader)};
            static_cast<void>(expectEnd());
            break;
        default:
            if (isExtensionType(frame.type)) {
                const auto body = reader.readBytes(reader.remainingSize());
                frame.request = ExtensionRequest{.type = frame.type, .body = Bytes(body.begin(), body.end())};
            } else {
                frame.problem = ErrorCode::kUnsupported;
            }
            break;
    }
    return frame;
}

Bytes encodeReply(const std::uint32_t id, const Reply& reply) {
    Bytes buffer(protocol::kMaxPacketSize);
    bytes::ByteWriter writer{buffer};
    writer.writeU8(replyType(reply));
    writer.writeU32(id);
    std::visit(
        [&writer](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, HelloReply>) {
                writer.writeU8(value.version);
            } else if constexpr (std::is_same_v<T, StatusReply>) {
                writer.writeU32(value.tick);
                writer.writeU32(value.entityCount);
                writer.writeU32(value.clientCount);
                writer.writeU64(value.uptimeMs);
                writer.writeU64(value.datagramsReceived);
                writer.writeU64(value.datagramsSent);
                writer.writeU64(value.bytesReceived);
                writer.writeU64(value.bytesSent);
            } else if constexpr (std::is_same_v<T, ClientsReply>) {
                const std::size_t count = std::min(value.clients.size(), maxClientEntries());
                writer.writeU32(value.total);
                writer.writeU16(static_cast<std::uint16_t>(count));
                for (std::size_t i = 0; i < count; ++i) {
                    writeEntry(writer, value.clients.at(i));
                }
            } else if constexpr (std::is_same_v<T, KickReply>) {
                writer.writeU32(value.client);
            } else if constexpr (std::is_same_v<T, SimulationReply>) {
                writeSettings(writer, value.settings);
            } else if constexpr (std::is_same_v<T, ErrorReply>) {
                writer.writeU8(static_cast<std::uint8_t>(value.code));
            } else {
                writer.writeBytes(value.body);
            }
        },
        reply);
    if (!writer.isValid()) {
        return {};
    }
    const auto written = writer.writtenBytes();
    return {written.begin(), written.end()};
}

std::optional<DecodedReply> decodeReply(const std::span<const std::byte> datagram) {
    bytes::ByteReader reader{datagram};
    const std::uint8_t type = reader.readU8();
    DecodedReply decoded;
    decoded.id = reader.readU32();
    if (!reader.isValid()) {
        return std::nullopt;
    }
    const auto finish = [&reader, &decoded](Reply reply) -> std::optional<DecodedReply> {
        if (!reader.isValid() || reader.remainingSize() != 0) {
            return std::nullopt;
        }
        decoded.reply = std::move(reply);
        return decoded;
    };
    if (type == toByte(MessageType::kError)) {
        const std::uint8_t code = reader.readU8();
        if (code < static_cast<std::uint8_t>(ErrorCode::kBadToken) ||
            code > static_cast<std::uint8_t>(ErrorCode::kBadValue)) {
            return std::nullopt;
        }
        return finish(ErrorReply{.code = static_cast<ErrorCode>(code)});
    }
    if ((type & kReplyFlag) == 0) {
        return std::nullopt;
    }
    switch (static_cast<std::uint8_t>(type & 0x7FU)) {
        case static_cast<std::uint8_t>(MessageType::kHello):
            return finish(HelloReply{.version = reader.readU8()});
        case static_cast<std::uint8_t>(MessageType::kStatus): {
            StatusReply status;
            status.tick = reader.readU32();
            status.entityCount = reader.readU32();
            status.clientCount = reader.readU32();
            status.uptimeMs = reader.readU64();
            status.datagramsReceived = reader.readU64();
            status.datagramsSent = reader.readU64();
            status.bytesReceived = reader.readU64();
            status.bytesSent = reader.readU64();
            return finish(status);
        }
        case static_cast<std::uint8_t>(MessageType::kClients): {
            ClientsReply list;
            list.total = reader.readU32();
            const std::uint16_t count = reader.readU16();
            if (count > maxClientEntries()) {
                return std::nullopt;
            }
            for (std::uint16_t i = 0; i < count; ++i) {
                const auto entry = readEntry(reader);
                if (!entry) {
                    return std::nullopt;
                }
                list.clients.push_back(*entry);
            }
            return finish(std::move(list));
        }
        case static_cast<std::uint8_t>(MessageType::kKick):
            return finish(KickReply{.client = reader.readU32()});
        case static_cast<std::uint8_t>(MessageType::kSimulationGet): {
            const simulation::SimulationSettings settings = readSettings(reader);
            return finish(SimulationReply{.settings = settings});
        }
        default:
            if (isExtensionType(static_cast<std::uint8_t>(type & 0x7FU))) {
                const auto body = reader.readBytes(reader.remainingSize());
                return finish(ExtensionReply{.type = type, .body = Bytes(body.begin(), body.end())});
            }
            return std::nullopt;
    }
}
}  // namespace rtype::network::admin
