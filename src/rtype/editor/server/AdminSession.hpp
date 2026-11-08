/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminSession
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "editor/Export.hpp"
#include "rtype/network/admin/AdminClient.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace rtype::editor::server {

/// @brief The editor's side of the admin channel of a running server: asks for what the panel shows, keeps the last
/// answers, and sends the commands (kick, simulation, world edits).
///
/// @details Requests are datagrams and may be lost, so a request with no reply is sent again after kRetryDelay, up to
/// kMaxAttempts times. A request that changes something and would do it twice (spawn, step) is never sent again. The
/// session asks for the status and the clients every kPollInterval by itself; the entity list and one entity's
/// components are asked for only while the panel keeps watching them (watchEntities(), watchEntity()). The clock is
/// given to update(), so a test drives the time. Not thread-safe.
class RTYPE_EDITOR_API AdminSession {
  public:
    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::milliseconds kRetryDelay{300};    ///< Before a request is sent again.
    static constexpr int kMaxAttempts = 3;                          ///< Times a request is sent before it is given up.
    static constexpr std::chrono::milliseconds kPollInterval{500};  ///< Between two polls of the status.
    static constexpr std::chrono::milliseconds kLostAfter{3000};    ///< Without a reply, a server seen is lost.
    static constexpr std::chrono::milliseconds kWatchFor{2000};     ///< How long a watch lasts without being renewed.

    /// @param port The admin port of the server, on `127.0.0.1`.
    /// @param token The secret of the server.
    /// @throws network::exceptions::SocketException if no socket can be opened.
    AdminSession(std::uint16_t port, std::string token);

    /// @brief Collects the replies, sends the requests again and polls. Call once a frame. Never blocks.
    void update(Clock::time_point now);

    /// @return Whether the server answered at least once.
    [[nodiscard]] bool isConnected() const noexcept { return _connected; }

    /// @return Whether the server answered once, but not for kLostAfter.
    [[nodiscard]] bool isLost() const noexcept { return _lost; }

    /// @return The last state of the server, nothing before the first answer.
    [[nodiscard]] const std::optional<network::admin::StatusReply>& status() const noexcept { return _status; }
    /// @return The last client list.
    [[nodiscard]] const std::optional<network::admin::ClientsReply>& clients() const noexcept { return _clients; }
    /// @return The link simulation in force, as the server last said.
    [[nodiscard]] const std::optional<network::simulation::SimulationSettings>& simulation() const noexcept {
        return _simulation;
    }
    /// @return The last entity page asked with watchEntities().
    [[nodiscard]] const std::optional<network::admin::EntitiesReply>& entities() const noexcept { return _entities; }
    /// @return The last answer of watchEntity().
    [[nodiscard]] const std::optional<network::admin::InspectReply>& inspected() const noexcept { return _inspected; }
    /// @return Whether the server's simulation is paused, nothing before the first answer.
    [[nodiscard]] std::optional<bool> isPaused() const noexcept { return _paused; }
    /// @return The entity the last spawn made.
    [[nodiscard]] std::optional<std::uint64_t> lastSpawned() const noexcept { return _lastSpawned; }
    /// @return What went wrong with the last command ("" when nothing did).
    [[nodiscard]] const std::string& lastError() const noexcept { return _lastError; }
    /// @return How many requests are waiting for a reply.
    [[nodiscard]] std::size_t pendingCount() const noexcept { return _pending.size(); }

    /// @brief Keeps the entity list, from the entity index @p first, up to date for a while.
    void watchEntities(std::uint32_t first);
    /// @brief Keeps the components of @p entity, from the component index @p firstComponent, up to date for a while.
    void watchEntity(std::uint64_t entity, std::uint16_t firstComponent);

    /// @brief Disconnects the client @p client.
    void kick(network::ClientId client);
    /// @brief Sets the link simulation of the server (it brings the values into their ranges and says them back).
    void setSimulation(const network::simulation::SimulationSettings& settings);
    /// @brief Sets one value of one component of an entity.
    void setField(const network::admin::SetFieldRequest& request);
    /// @brief Spawns an entity named @p name.
    void spawn(const std::string& name);
    /// @brief Despawns an entity.
    void despawn(std::uint64_t entity);
    /// @brief Pauses, resumes or steps the server's simulation.
    void run(network::admin::RunAction action);

  private:
    /// @brief What a request asks, to file its reply and to decide whether it is sent again.
    enum class Kind : std::uint8_t {
        kStatus,
        kClients,
        kSimulationGet,
        kSimulationSet,
        kKick,
        kEntities,
        kInspect,
        kSetField,
        kSpawn,
        kDespawn,
        kRun,
        kQuery,  ///< A run request that only asks.
    };

    /// @brief A request waiting for its reply.
    struct Pending {
        Kind kind{Kind::kStatus};
        network::admin::Request request;
        Clock::time_point sentAt;
        int attempts{1};
    };

    /// @brief Sends @p request and waits for its reply.
    void submit(Kind kind, const network::admin::Request& request);
    /// @brief Sends a world request.
    void submitWorld(Kind kind, const network::admin::WorldRequest& request);
    [[nodiscard]] bool hasPending(Kind kind) const;
    [[nodiscard]] static bool isRetryable(Kind kind) noexcept;
    void handle(const network::admin::DecodedReply& decoded);
    void handleWorld(const network::admin::ExtensionReply& reply);
    void retry();
    void poll();

    network::admin::AdminClient _client;
    Clock::time_point _now;
    std::map<std::uint32_t, Pending> _pending;  ///< By request id.
    bool _connected{false};
    bool _lost{false};
    Clock::time_point _lastReply;
    Clock::time_point _nextPoll;
    std::optional<network::admin::StatusReply> _status;
    std::optional<network::admin::ClientsReply> _clients;
    std::optional<network::simulation::SimulationSettings> _simulation;
    std::optional<network::admin::EntitiesReply> _entities;
    std::optional<network::admin::InspectReply> _inspected;
    std::optional<bool> _paused;
    std::optional<std::uint64_t> _lastSpawned;
    std::string _lastError;
    std::optional<Clock::time_point> _watchEntitiesAt;
    std::uint32_t _entitiesFirst{0};
    std::optional<Clock::time_point> _watchEntityAt;
    std::uint64_t _watchedEntity{0};
    std::uint16_t _watchedFirstComponent{0};
};

}  // namespace rtype::editor::server
