/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReliableSender
*/

#include "rtype/network/connection/ReliableSender.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/protocol/Constants.hpp"

namespace rtype::network::connection {
std::uint16_t ReliableSender::push(Bytes payload) {
    if (_overflowed) {
        return _nextId;
    }
    PendingMessage& message = _pending.emplace_back();
    message.id = _nextId;
    message.payload = std::move(payload);
    ++_unacknowledged;
    if (_unacknowledged > protocol::kMaxPendingReliable) {
        _overflowed = true;
    }
    return _nextId++;
}

std::vector<const PendingMessage*> ReliableSender::collectDue(const TimePoint now,
                                                              const std::chrono::milliseconds interval) const {
    std::vector<const PendingMessage*> due;
    if (_pending.empty()) {
        return due;
    }
    const std::uint16_t oldest = _pending.front().id;
    for (const PendingMessage& message : _pending) {
        if (static_cast<std::uint16_t>(message.id - oldest) >= protocol::kReliableWindow) {
            break;
        }
        if (!message.acknowledged && (!message.sent || now - message.lastSendTime >= interval)) {
            due.push_back(&message);
        }
    }
    return due;
}

void ReliableSender::markSent(const std::uint16_t id, const TimePoint now) noexcept {
    if (PendingMessage* message = find(id)) {
        message->sent = true;
        message->lastSendTime = now;
    }
}

void ReliableSender::acknowledge(const std::uint16_t id) noexcept {
    PendingMessage* message = find(id);
    if (message == nullptr || message->acknowledged) {
        return;
    }
    message->acknowledged = true;
    message->payload = {};
    --_unacknowledged;
    while (!_pending.empty() && _pending.front().acknowledged) {
        _pending.pop_front();
    }
}

bool ReliableSender::isOverflowed() const noexcept { return _overflowed; }

std::size_t ReliableSender::pendingCount() const noexcept { return _unacknowledged; }

PendingMessage* ReliableSender::find(const std::uint16_t id) noexcept {
    if (_pending.empty()) {
        return nullptr;
    }
    const auto index = static_cast<std::uint16_t>(id - _pending.front().id);
    return index < _pending.size() ? &*(_pending.begin() + index) : nullptr;
}
}  // namespace rtype::network::connection
