/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UdpSocket
*/

#include "rtype/network/io/UdpSocket.hpp"

#include <asio.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "rtype/network/exceptions/NetworkExceptions.hpp"
#include "rtype/network/protocol/Constants.hpp"

#ifdef _WIN32
#include <mswsock.h>
#include <winsock2.h>
#endif

namespace rtype::network::io {
namespace {
Address toAddress(const asio::ip::udp::endpoint& endpoint) {
    const auto ip = endpoint.address();
    const auto v6 = ip.is_v4() ? asio::ip::make_address_v6(asio::ip::v4_mapped, ip.to_v4()) : ip.to_v6();
    return Address{v6.to_bytes(), endpoint.port()};
}
}  // namespace

struct UdpSocket::Impl {
    asio::io_context context;
    asio::ip::udp::socket socket{context};
    asio::ip::udp::endpoint sender;
    std::vector<std::byte> buffer = std::vector<std::byte>(protocol::kMaxPacketSize + 1);
    std::vector<Datagram> received;
    SocketStats stats;

    void armReceive() {
        socket.async_receive_from(
            asio::buffer(buffer), sender,
            [this](const asio::error_code& error, const std::size_t size) { onReceive(error, size); });
    }

    void onReceive(const asio::error_code& error, const std::size_t size) {
        if (error == asio::error::operation_aborted) {
            return;
        }
        if (error) {
            ++stats.receiveErrors;
        } else if (size > protocol::kMaxPacketSize) {
            ++stats.oversized;
        } else {
            ++stats.received;
            received.push_back(Datagram{toAddress(sender), Bytes(buffer.begin(), buffer.begin() + size)});
        }
        armReceive();
    }
};

UdpSocket::UdpSocket(const std::uint16_t port) : _impl{std::make_unique<Impl>()} {
    asio::error_code error;
    static_cast<void>(_impl->socket.open(asio::ip::udp::v4(), error));
    if (!error) {
        static_cast<void>(_impl->socket.bind(asio::ip::udp::endpoint{asio::ip::udp::v4(), port}, error));
    }
    if (error) {
        throw exceptions::SocketException{port, error.message()};
    }
#ifdef _WIN32
    // A datagram sent to a closed port would make the next receive fail with WSAECONNRESET.
    BOOL reportConnectionReset = FALSE;
    DWORD bytesReturned = 0;
    WSAIoctl(_impl->socket.native_handle(), SIO_UDP_CONNRESET, &reportConnectionReset, sizeof(reportConnectionReset),
             nullptr, 0, &bytesReturned, nullptr, nullptr);
#endif
    _impl->armReceive();
}

UdpSocket::~UdpSocket() = default;

void UdpSocket::sendTo(const Address& to, const std::span<const std::byte> bytes) noexcept {
    try {
        const asio::ip::address_v6 v6{to.bytes()};
        if (!v6.is_v4_mapped()) {
            ++_impl->stats.sendErrors;
            return;
        }
        const asio::ip::udp::endpoint destination{asio::ip::make_address_v4(asio::ip::v4_mapped, v6), to.port()};
        asio::error_code error;
        static_cast<void>(_impl->socket.send_to(asio::buffer(bytes.data(), bytes.size()), destination, 0, error));
        if (error) {
            ++_impl->stats.sendErrors;
        }
    } catch (...) {
        ++_impl->stats.sendErrors;
    }
}

void UdpSocket::poll() {
    if (_impl->context.stopped()) {
        _impl->context.restart();
    }
    static_cast<void>(_impl->context.poll());
}

void UdpSocket::run() {
    if (_impl->context.stopped()) {
        _impl->context.restart();
    }
    static_cast<void>(_impl->context.run());
}

void UdpSocket::stop() noexcept {
    asio::error_code error;
    static_cast<void>(_impl->socket.cancel(error));
    _impl->context.stop();
}

std::vector<Datagram> UdpSocket::takeReceived() { return std::exchange(_impl->received, {}); }

std::uint16_t UdpSocket::localPort() const { return _impl->socket.local_endpoint().port(); }

SocketStats UdpSocket::stats() const noexcept { return _impl->stats; }
}  // namespace rtype::network::io
