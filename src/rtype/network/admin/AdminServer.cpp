/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminServer
*/

#include "rtype/network/admin/AdminServer.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/io/SocketStats.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/server/ClientInfo.hpp"
#include "rtype/network/server/NetServer.hpp"
#include "rtype/network/server/RoomMessages.hpp"

namespace rtype::network::admin {
namespace {
/// @return Whether both texts are equal, in a time that depends on their lengths only.
bool sameToken(const std::string& expected, const std::string& given) noexcept {
    std::size_t difference = expected.size() ^ given.size();
    for (std::size_t i = 0; i < expected.size() && i < given.size(); ++i) {
        difference |= static_cast<std::size_t>(static_cast<std::uint8_t>(expected.at(i)) ^
                                               static_cast<std::uint8_t>(given.at(i)));
    }
    return difference == 0;
}
}  // namespace

AdminServer::AdminServer(const std::uint16_t port, std::string token, const TimePoint started)
    : _socket{port, true}, _token{std::move(token)}, _started{started} {}

void AdminServer::setExtensionHandler(ExtensionHandler handler) { _extension = std::move(handler); }

std::uint16_t AdminServer::localPort() const { return _socket.localPort(); }

void AdminServer::poll(const TimePoint now, server::NetServer& server, const GameStatus& game) {
    _socket.poll();
    for (const io::Datagram& datagram : _socket.takeReceived()) {
        const std::optional<RequestFrame> frame = decodeRequest(datagram.bytes);
        if (!frame) {
            continue;
        }
        const Reply reply = answer(*frame, now, server, game);
        const Bytes bytes = encodeReply(frame->id, reply);
        if (!bytes.empty()) {
            _socket.sendTo(datagram.from, bytes);
        }
    }
}

Reply AdminServer::answer(const RequestFrame& frame, const TimePoint now, server::NetServer& server,
                          const GameStatus& game) {
    if (_token.empty() || !sameToken(_token, frame.token)) {
        return ErrorReply{.code = ErrorCode::kBadToken};
    }
    if (!frame.request) {
        return ErrorReply{.code = frame.problem};
    }
    if (std::holds_alternative<HelloRequest>(*frame.request)) {
        return HelloReply{};
    }
    if (std::holds_alternative<StatusRequest>(*frame.request)) {
        const io::SocketStats stats = server.socketStats();
        const auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(now - _started);
        return StatusReply{
            .tick = game.tick,
            .entityCount = game.entityCount,
            .clientCount = static_cast<std::uint32_t>(server.core().sessionCount()),
            .uptimeMs = static_cast<std::uint64_t>(std::max<std::int64_t>(0, uptime.count())),
            .datagramsReceived = stats.received,
            .datagramsSent = stats.sent,
            .bytesReceived = stats.bytesReceived,
            .bytesSent = stats.bytesSent,
        };
    }
    if (std::holds_alternative<ClientsRequest>(*frame.request)) {
        ClientsReply list;
        const std::vector<server::ClientInfo> clients = server.clients();
        list.total = static_cast<std::uint32_t>(clients.size());
        for (const server::ClientInfo& info : clients) {
            if (list.clients.size() >= maxClientEntries()) {
                break;
            }
            list.clients.push_back(ClientEntry{
                .client = info.client,
                .state = info.state,
                .address = info.address,
                .rttMs = info.rttMs,
                .droppedMessages = info.droppedMessages,
                .protocolErrors = info.protocolErrors,
            });
        }
        return list;
    }
    if (const auto* kick = std::get_if<KickRequest>(&*frame.request)) {
        if (!server.core().hasSession(kick->client)) {
            return ErrorReply{.code = ErrorCode::kUnknownClient};
        }
        server.execute(server::KickClient{.client = kick->client, .reason = DisconnectReason::kKicked});
        return KickReply{.client = kick->client};
    }
    if (std::holds_alternative<SimulationGetRequest>(*frame.request)) {
        return SimulationReply{.settings = server.simulation()};
    }
    if (const auto* set = std::get_if<SimulationSetRequest>(&*frame.request)) {
        return SimulationReply{.settings = server.setSimulation(set->settings)};
    }
    const auto& extension = std::get<ExtensionRequest>(*frame.request);
    if (_extension) {
        if (std::optional<Reply> reply = _extension(extension)) {
            return std::move(*reply);
        }
    }
    return ErrorReply{.code = ErrorCode::kUnsupported};
}
}  // namespace rtype::network::admin
