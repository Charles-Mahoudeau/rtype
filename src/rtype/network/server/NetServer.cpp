/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetServer
*/

#include "rtype/network/server/NetServer.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/server/RoomMessages.hpp"

namespace rtype::network::server {
NetServer::NetServer(const NetworkSettings& settings) : _socket{settings.port}, _core{settings, makeSystemRandom()} {}

void NetServer::setWelcomeMessages(std::vector<Bytes> messages) { _core.setWelcomeMessages(std::move(messages)); }

void NetServer::poll(const TimePoint now) {
    _socket.poll();
    for (const auto& [from, bytes] : _socket.takeReceived()) {
        _core.receive(from, bytes, now);
    }
    _core.update(now);
    flush(now);
}

void NetServer::execute(ServerCommand command) { _core.execute(std::move(command)); }

void NetServer::flush(const TimePoint now) {
    for (const auto& [to, bytes] : _core.takeDatagrams(now)) {
        _socket.sendTo(to, bytes);
    }
}

void NetServer::shutdown(const TimePoint now) {
    _core.shutdown(now);
    flush(now);
}

std::vector<ServerEvent> NetServer::takeEvents() { return _core.takeEvents(); }

std::uint16_t NetServer::localPort() const { return _socket.localPort(); }

const ServerCore& NetServer::core() const noexcept { return _core; }
}  // namespace rtype::network::server
