/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminClient
*/

#include "rtype/network/admin/AdminClient.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/UdpSocket.hpp"

namespace rtype::network::admin {
AdminClient::AdminClient(const std::uint16_t port, std::string token)
    : _socket{0, true},
      _server{io::Address::resolve("127.0.0.1", port).value_or(io::Address{})},
      _token{std::move(token)} {}

std::uint32_t AdminClient::send(const Request& request) {
    const std::uint32_t id = _nextId;
    const Bytes bytes = encodeRequest(id, _token, request);
    if (bytes.empty()) {
        return 0;
    }
    ++_nextId;
    _socket.sendTo(_server, bytes);
    return id;
}

std::vector<DecodedReply> AdminClient::poll() {
    std::vector<DecodedReply> replies;
    _socket.poll();
    for (const io::Datagram& datagram : _socket.takeReceived()) {
        if (datagram.from != _server) {
            continue;
        }
        if (std::optional<DecodedReply> reply = decodeReply(datagram.bytes)) {
            replies.push_back(std::move(*reply));
        }
    }
    return replies;
}
}  // namespace rtype::network::admin
