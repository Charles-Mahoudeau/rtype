/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReliableSender
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/Types.hpp"

namespace rtype::network::connection {
/// @brief A reliable message waiting for the acknowledgement of the peer.
struct PendingMessage {
    std::uint16_t id{0};       ///< Identifies the message for the receiver.
    Bytes payload;             ///< The bytes of the message.
    bool sent{false};          ///< True once it went into a datagram.
    TimePoint lastSendTime;    ///< When it last went into a datagram; meaningful only if `sent`.
    bool acknowledged{false};  ///< True once a datagram that carried it was acked.
};

/// @brief Decides which reliable messages go into the next datagrams: those never sent and those not acknowledged
/// for too long.
///
/// @details Messages get consecutive ids and stay pending until acknowledged. Only the messages within
/// `kReliableWindow` ids of the oldest unacknowledged one may be sent, so ids stay comparable across the wrap. It
/// never throws: when too many messages are pending it stops storing and reports `isOverflowed`.
class RTYPE_NETWORK_API ReliableSender {
  public:
    /// @brief Builds a sender with nothing pending, whose first message gets the id 0.
    ReliableSender() = default;

    /// @brief Destroys the sender and the messages it still holds.
    ~ReliableSender() = default;

    /// @brief Copies the pending messages.
    ReliableSender(const ReliableSender&) = default;

    /// @brief Copy-assigns the pending messages.
    ReliableSender& operator=(const ReliableSender&) = default;

    /// @brief Moves the pending messages.
    ReliableSender(ReliableSender&&) noexcept = default;

    /// @brief Move-assigns the pending messages.
    ReliableSender& operator=(ReliableSender&&) noexcept = default;

    /// @brief Adds a message to the pending list and gives it the next id.
    ///
    /// @details Once more than `kMaxPendingReliable` messages are pending the sender is overflowed: it stores nothing
    /// more.
    /// @param payload The bytes of the message.
    /// @return The id given, or the id the next message would get if the sender is overflowed and nothing was stored.
    std::uint16_t push(Bytes payload);

    /// @brief Lists the messages that should go into a datagram now.
    /// @param now The current time.
    /// @param interval The delay after which a message already sent is due again.
    /// @return The due messages within the window, oldest first. The pointers stay valid until the next call to
    /// `push` or `acknowledge`.
    [[nodiscard]] std::vector<const PendingMessage*> collectDue(TimePoint now,
                                                                std::chrono::milliseconds interval) const;

    /// @brief Records that a message went into a datagram.
    /// @param id The id of the message; an unknown id is ignored.
    /// @param now The time the datagram was built.
    void markSent(std::uint16_t id, TimePoint now) noexcept;

    /// @brief Forgets a message the peer confirmed; nothing happens if it is unknown or already acknowledged.
    /// @param id The id of the message.
    void acknowledge(std::uint16_t id) noexcept;

    /// @brief Tells whether the peer failed to keep up.
    /// @return True once more than `kMaxPendingReliable` messages were pending at the same time, and for good.
    [[nodiscard]] bool isOverflowed() const noexcept;

    /// @brief Gives how many messages still wait for an acknowledgement.
    /// @return The number of pending messages.
    [[nodiscard]] std::size_t pendingCount() const noexcept;

  private:
    /// @brief Finds a pending message by id.
    [[nodiscard]] PendingMessage* find(std::uint16_t id) noexcept;

    std::deque<PendingMessage> _pending;  ///< By increasing id; the front is the oldest unacknowledged message.
    std::uint16_t _nextId{0};             ///< Id of the next message pushed.
    std::size_t _unacknowledged{0};       ///< Messages of `_pending` not acknowledged yet.
    bool _overflowed{false};              ///< Set for good when too many messages are pending.
};
}  // namespace rtype::network::connection
