/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReliableReceiver
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/SequenceBuffer.hpp"

namespace rtype::network::connection {
/// @brief Delivers the reliable messages of a peer exactly once and in id order, whatever the order they arrive in
/// and however many times they are resent.
class RTYPE_NETWORK_API ReliableReceiver {
  public:
    /// @brief Builds a receiver that expects the id 0.
    ReliableReceiver() = default;

    /// @brief Destroys the receiver and the messages it still holds.
    ~ReliableReceiver() = default;

    /// @brief Copies the state.
    ReliableReceiver(const ReliableReceiver&) = default;

    /// @brief Copy-assigns the state.
    ReliableReceiver& operator=(const ReliableReceiver&) = default;

    /// @brief Moves the state.
    ReliableReceiver(ReliableReceiver&&) noexcept = default;

    /// @brief Move-assigns the state.
    ReliableReceiver& operator=(ReliableReceiver&&) noexcept = default;

    /// @brief Takes one message received.
    ///
    /// @details A message older than the expected id is a duplicate from a resend: it is dropped without error.
    /// @param id The id of the message.
    /// @param payload The bytes of the message; copied if kept.
    /// @return False if the id is `kReliableWindow` or more ahead of the expected one: a correct peer never does
    /// that, the message is dropped.
    bool receive(std::uint16_t id, std::span<const std::byte> payload);

    /// @brief Gives the messages that became deliverable since the last call, in id order, and forgets them.
    /// @return The messages.
    [[nodiscard]] std::vector<Bytes> takeDelivered();

  private:
    std::uint16_t _expected{0};                                         ///< Id of the next message to deliver.
    protocol::SequenceBuffer<Bytes, protocol::kReliableWindow> _early;  ///< Messages kept aside, by id.
    std::vector<Bytes> _delivered;                                      ///< Messages to take.
};
}  // namespace rtype::network::connection
