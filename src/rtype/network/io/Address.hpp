/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Address
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "rtype/network/Export.hpp"

namespace rtype::network::io {
/// @brief Names a peer: an IP address and a port. Hides Asio's endpoint so no header needs Asio.
///
/// @details IPv4 addresses are stored as IPv4-mapped IPv6, so one IP has one representation. A default-constructed
/// address is `0.0.0.0:0` (stored as the unspecified address `::`), which no real peer has.
class RTYPE_NETWORK_API Address {
  public:
    /// @brief The 16 bytes of an IPv6 address, in network order.
    using Bytes16 = std::array<std::uint8_t, 16>;

    /// @brief Builds an address from raw bytes.
    /// @param bytes An IPv6 address, or an IPv4 address mapped into IPv6.
    /// @param port The port.
    Address(const Bytes16& bytes, std::uint16_t port) noexcept;

    /// @brief Builds the address `0.0.0.0:0`.
    Address() = default;

    /// @brief Destroys the address.
    ~Address() = default;

    /// @brief Copies the address.
    Address(const Address&) = default;

    /// @brief Copy-assigns the address.
    Address& operator=(const Address&) = default;

    /// @brief Moves the address.
    Address(Address&&) noexcept = default;

    /// @brief Move-assigns the address.
    Address& operator=(Address&&) noexcept = default;

    /// @brief Turns a host name or an IP literal into an address. Blocking: call it at startup only.
    /// @param host A host name or an IP literal.
    /// @param port The port of the address.
    /// @return The address, IPv4 preferred; `std::nullopt` if nothing resolves.
    [[nodiscard]] static std::optional<Address> resolve(std::string_view host, std::uint16_t port);

    /// @brief Gives the IP as 16 bytes (IPv4 mapped into IPv6).
    [[nodiscard]] const Bytes16& bytes() const noexcept;

    /// @brief Gives the port.
    [[nodiscard]] std::uint16_t port() const noexcept;

    /// @brief Formats the address for logs.
    /// @return `"127.0.0.1:4242"` for IPv4, `"[::1]:4242"` for IPv6.
    [[nodiscard]] std::string string() const;

    /// @brief Tells whether both IP and port are equal.
    friend bool operator==(const Address& left, const Address& right) noexcept = default;

  private:
    Bytes16 _bytes{};        ///< IPv6, or IPv4 mapped into IPv6.
    std::uint16_t _port{0};  ///< The port.
};
}  // namespace rtype::network::io

/// @brief Hash consistent with `Address::operator==`.
template <>
struct RTYPE_NETWORK_API std::hash<rtype::network::io::Address> {
    [[nodiscard]] std::size_t operator()(const rtype::network::io::Address& address) const noexcept;
};
