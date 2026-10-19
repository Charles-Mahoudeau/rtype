/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Address
*/

#include "rtype/network/io/Address.hpp"

#include <asio.hpp>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace rtype::network::io {
Address::Address(const Bytes16& bytes, const std::uint16_t port) noexcept : _bytes{bytes}, _port{port} {}

std::optional<Address> Address::resolve(const std::string_view host, const std::uint16_t port) {
    try {
        asio::io_context context;
        asio::ip::udp::resolver resolver{context};
        asio::error_code error;
        const auto results = resolver.resolve(host, std::to_string(port), error);
        if (error) {
            return std::nullopt;
        }
        std::optional<asio::ip::address> chosen;
        for (const auto& entry : results) {
            const auto ip = entry.endpoint().address();
            if (ip.is_v4()) {
                chosen = ip;
                break;
            }
            if (!chosen) {
                chosen = ip;
            }
        }
        if (!chosen) {
            return std::nullopt;
        }
        const auto v6 =
            chosen->is_v4() ? asio::ip::make_address_v6(asio::ip::v4_mapped, chosen->to_v4()) : chosen->to_v6();
        return Address{v6.to_bytes(), port};
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

const Address::Bytes16& Address::bytes() const noexcept { return _bytes; }

std::uint16_t Address::port() const noexcept { return _port; }

std::string Address::string() const {
    const asio::ip::address_v6 v6{_bytes};
    if (v6.is_v4_mapped()) {
        return std::format("{}:{}", asio::ip::make_address_v4(asio::ip::v4_mapped, v6).to_string(), _port);
    }
    return std::format("[{}]:{}", v6.to_string(), _port);
}
}  // namespace rtype::network::io

std::size_t std::hash<rtype::network::io::Address>::operator()(
    const rtype::network::io::Address& address) const noexcept {
    // FNV-1a over the IP bytes and the port.
    std::size_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](const std::uint8_t byte) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    };
    for (const auto byte : address.bytes()) {
        mix(byte);
    }
    mix(static_cast<std::uint8_t>(address.port() & 0xFFU));
    mix(static_cast<std::uint8_t>(address.port() >> 8U));
    return hash;
}
