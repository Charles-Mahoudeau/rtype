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
#include "rtype/network/io/SocketStats.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/server/ClientInfo.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace rtype::network::server {
NetServer::NetServer(const NetworkSettings& settings) : NetServer{settings, makeSystemRandom()} {}

NetServer::NetServer(const NetworkSettings& settings, RandomSource simulationRandom)
    : _socket{settings.port},
      _core{settings, makeSystemRandom()},
      _inbound{simulationRandom},
      _outbound{std::move(simulationRandom)} {}

void NetServer::setWelcomeMessages(std::vector<Bytes> messages) { _core.setWelcomeMessages(std::move(messages)); }

void NetServer::poll(const TimePoint now) {
    _socket.poll();
    if (_inbound.isIdle()) {
        for (const auto& [from, bytes] : _socket.takeReceived()) {
            _core.receive(from, bytes, now);
        }
    } else {
        for (io::Datagram& datagram : _socket.takeReceived()) {
            _inbound.submit({.peer = datagram.from, .bytes = std::move(datagram.bytes)}, now);
        }
        for (const auto& [from, bytes] : _inbound.takeReady(now)) {
            _core.receive(from, bytes, now);
        }
    }
    _core.update(now);
    flush(now);
}

bool NetServer::execute(ServerCommand command) { return _core.execute(std::move(command)); }

void NetServer::flush(const TimePoint now) {
    if (_outbound.isIdle()) {
        for (const auto& [to, bytes] : _core.takeDatagrams(now)) {
            _socket.sendTo(to, bytes);
        }
        return;
    }
    for (OutgoingDatagram& datagram : _core.takeDatagrams(now)) {
        _outbound.submit({.peer = datagram.to, .bytes = std::move(datagram.bytes)}, now);
    }
    for (const auto& [to, bytes] : _outbound.takeReady(now)) {
        _socket.sendTo(to, bytes);
    }
}

void NetServer::shutdown(const TimePoint now) {
    _core.shutdown(now);
    // The server is about to stop: what the simulation holds leaves now, with the goodbyes.
    for (const auto& [to, bytes] : _outbound.takeAll()) {
        _socket.sendTo(to, bytes);
    }
    for (const auto& [to, bytes] : _core.takeDatagrams(now)) {
        _socket.sendTo(to, bytes);
    }
}

std::vector<ServerEvent> NetServer::takeEvents() { return _core.takeEvents(); }

std::uint16_t NetServer::localPort() const { return _socket.localPort(); }

std::vector<ClientInfo> NetServer::clients() const { return _core.clients(); }

io::SocketStats NetServer::socketStats() const noexcept { return _socket.stats(); }

simulation::SimulationSettings NetServer::setSimulation(const simulation::SimulationSettings& settings) {
    _inbound.configure(settings);
    return _outbound.configure(settings);
}

const simulation::SimulationSettings& NetServer::simulation() const noexcept { return _outbound.settings(); }

const ServerCore& NetServer::core() const noexcept { return _core; }
}  // namespace rtype::network::server
