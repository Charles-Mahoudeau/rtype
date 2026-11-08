/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminSession
*/

#include "AdminSession.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace rtype::editor::server {

namespace admin = network::admin;

namespace {
/// @return A sentence for @p code.
std::string_view describe(const admin::ErrorCode code) {
    switch (code) {
        case admin::ErrorCode::kBadToken:
            return "the server refused the admin token";
        case admin::ErrorCode::kMalformed:
            return "the server could not read the request";
        case admin::ErrorCode::kUnsupported:
            return "the server does not support this request";
        case admin::ErrorCode::kUnknownClient:
            return "no such client";
        case admin::ErrorCode::kUnknownEntity:
            return "no such entity";
        case admin::ErrorCode::kUnknownComponent:
            return "no such component";
        case admin::ErrorCode::kUnknownField:
            return "no such field";
        case admin::ErrorCode::kBadValue:
            return "the value does not fit the field, or the field cannot be edited";
    }
    return "unknown error";
}
}  // namespace

AdminSession::AdminSession(const std::uint16_t port, std::string token)
    : _client{port, std::move(token)}, _now{Clock::now()}, _lastReply{_now}, _nextPoll{_now} {}

bool AdminSession::isRetryable(const Kind kind) noexcept {
    // A lost spawn or step sent again would happen twice; the others give the same result the second time.
    return kind != Kind::kSpawn && kind != Kind::kRun;
}

bool AdminSession::hasPending(const Kind kind) const {
    return std::ranges::any_of(_pending, [kind](const auto& entry) { return entry.second.kind == kind; });
}

void AdminSession::submit(const Kind kind, const admin::Request& request) {
    const std::uint32_t id = _client.send(request);
    if (id == 0) {
        _lastError = "the request is too large to send";
        return;
    }
    _pending.insert_or_assign(id, Pending{.kind = kind, .request = request, .sentAt = _now, .attempts = 1});
}

void AdminSession::submitWorld(const Kind kind, const admin::WorldRequest& request) {
    submit(kind, admin::Request{admin::encodeWorldRequest(request)});
}

void AdminSession::update(const Clock::time_point now) {
    _now = now;
    for (const admin::DecodedReply& decoded : _client.poll()) {
        handle(decoded);
    }
    retry();
    if (_connected && now - _lastReply > kLostAfter) {
        _lost = true;
    }
    if (now >= _nextPoll) {
        _nextPoll = now + kPollInterval;
        poll();
    }
}

void AdminSession::poll() {
    if (!hasPending(Kind::kStatus)) {
        submit(Kind::kStatus, admin::StatusRequest{});
    }
    if (!hasPending(Kind::kClients)) {
        submit(Kind::kClients, admin::ClientsRequest{});
    }
    if (!_simulation && !hasPending(Kind::kSimulationGet)) {
        submit(Kind::kSimulationGet, admin::SimulationGetRequest{});
    }
    if (!_paused && !hasPending(Kind::kQuery)) {
        submitWorld(Kind::kQuery, admin::RunRequest{admin::RunAction::kQuery});
    }
    if (_watchEntitiesAt && _now - *_watchEntitiesAt < kWatchFor && !hasPending(Kind::kEntities)) {
        submitWorld(Kind::kEntities, admin::EntitiesRequest{_entitiesFirst});
    }
    if (_watchEntityAt && _now - *_watchEntityAt < kWatchFor && !hasPending(Kind::kInspect)) {
        submitWorld(Kind::kInspect,
                    admin::InspectRequest{.entity = _watchedEntity, .firstComponent = _watchedFirstComponent});
    }
}

void AdminSession::retry() {
    std::vector<std::uint32_t> due;
    for (const auto& [id, pending] : _pending) {
        if (_now - pending.sentAt >= kRetryDelay) {
            due.push_back(id);
        }
    }
    for (const std::uint32_t id : due) {
        Pending pending = std::move(_pending.at(id));
        _pending.erase(id);
        if (pending.attempts >= kMaxAttempts || !isRetryable(pending.kind)) {
            // A poll gives up silently (the next poll asks again); a command the user gave tells it failed.
            if (pending.kind != Kind::kStatus && pending.kind != Kind::kClients && pending.kind != Kind::kQuery &&
                pending.kind != Kind::kSimulationGet && pending.kind != Kind::kEntities &&
                pending.kind != Kind::kInspect && _connected) {
                _lastError = "the server did not answer";
            }
            continue;
        }
        const std::uint32_t newId = _client.send(pending.request);
        if (newId == 0) {
            continue;
        }
        ++pending.attempts;
        pending.sentAt = _now;
        _pending.insert_or_assign(newId, std::move(pending));
    }
}

void AdminSession::handle(const admin::DecodedReply& decoded) {
    const auto found = _pending.find(decoded.id);
    if (found == _pending.end()) {
        return;  // a late reply to a request already answered or given up
    }
    const Kind kind = found->second.kind;
    _pending.erase(found);
    _connected = true;
    _lost = false;
    _lastReply = _now;

    struct Visitor {
        AdminSession* self;
        Kind kind;
        void operator()(const admin::HelloReply& /*reply*/) const {}
        void operator()(const admin::StatusReply& reply) const { self->_status = reply; }
        void operator()(const admin::ClientsReply& reply) const { self->_clients = reply; }
        void operator()(const admin::KickReply& /*reply*/) const {
            self->_lastError.clear();
            self->_nextPoll = self->_now;  // see the list change at once
        }
        void operator()(const admin::SimulationReply& reply) const {
            self->_simulation = reply.settings;
            if (kind == Kind::kSimulationSet) {
                self->_lastError.clear();
            }
        }
        void operator()(const admin::ErrorReply& reply) const {
            if (reply.code == admin::ErrorCode::kUnknownEntity && kind == Kind::kInspect) {
                self->_inspected.reset();  // the watched entity is gone
                return;
            }
            self->_lastError = describe(reply.code);
        }
        void operator()(const admin::ExtensionReply& reply) const { self->handleWorld(reply); }
    };
    std::visit(Visitor{.self = this, .kind = kind}, decoded.reply);
}

void AdminSession::handleWorld(const admin::ExtensionReply& reply) {
    const std::optional<admin::WorldReply> world = admin::decodeWorldReply(reply);
    if (!world) {
        _lastError = "the server sent a world reply that cannot be read";
        return;
    }
    struct Visitor {
        AdminSession* self;
        void operator()(const admin::EntitiesReply& value) const { self->_entities = value; }
        void operator()(const admin::InspectReply& value) const { self->_inspected = value; }
        void operator()(const admin::SetFieldReply& /*value*/) const {
            self->_lastError.clear();
            self->_nextPoll = self->_now;
        }
        void operator()(const admin::SpawnReply& value) const {
            self->_lastSpawned = value.entity;
            self->_lastError.clear();
            self->_nextPoll = self->_now;
        }
        void operator()(const admin::DespawnReply& /*value*/) const {
            self->_lastError.clear();
            self->_nextPoll = self->_now;
        }
        void operator()(const admin::RunReply& value) const { self->_paused = value.paused; }
    };
    std::visit(Visitor{.self = this}, *world);
}

void AdminSession::watchEntities(const std::uint32_t first) {
    _entitiesFirst = first;
    if (!_watchEntitiesAt || _now - *_watchEntitiesAt >= kWatchFor) {
        _nextPoll = _now;  // a new watch: ask now, not at the next poll
    }
    _watchEntitiesAt = _now;
}

void AdminSession::watchEntity(const std::uint64_t entity, const std::uint16_t firstComponent) {
    if (entity != _watchedEntity || firstComponent != _watchedFirstComponent) {
        _inspected.reset();
        _nextPoll = _now;
    }
    _watchedEntity = entity;
    _watchedFirstComponent = firstComponent;
    _watchEntityAt = _now;
}

void AdminSession::kick(const network::ClientId client) { submit(Kind::kKick, admin::KickRequest{client}); }

void AdminSession::setSimulation(const network::simulation::SimulationSettings& settings) {
    submit(Kind::kSimulationSet, admin::SimulationSetRequest{settings});
}

void AdminSession::setField(const admin::SetFieldRequest& request) { submitWorld(Kind::kSetField, request); }

void AdminSession::spawn(const std::string& name) { submitWorld(Kind::kSpawn, admin::SpawnRequest{name}); }

void AdminSession::despawn(const std::uint64_t entity) { submitWorld(Kind::kDespawn, admin::DespawnRequest{entity}); }

void AdminSession::run(const admin::RunAction action) { submitWorld(Kind::kRun, admin::RunRequest{action}); }

}  // namespace rtype::editor::server
