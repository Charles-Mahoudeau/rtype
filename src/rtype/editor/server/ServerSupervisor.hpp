/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerSupervisor
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "editor/Export.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/process/ChildProcess.hpp"
#include "editor/server/AdminSession.hpp"

namespace rtype::editor::server {

/// @brief How to start the server.
struct ServerOptions {
    std::filesystem::path gameDirectory;  ///< The game folder (`project.luau`, `scripts/`, `assets/`).
    std::uint16_t port{4242};             ///< The port the clients connect to, from 1 to 65535.
    std::vector<std::string> extraArgs;   ///< More flags for the game (`engine.args`).
    std::filesystem::path runtime;        ///< The `aylo-runtime` to run; empty: the one next to the editor.
};

/// @brief What the server is doing, as the editor knows it.
enum class ServerStatus : std::uint8_t {
    kStopped,   ///< Never started, or stopped by the user.
    kStarting,  ///< The process runs, the admin channel has not answered yet.
    kRunning,   ///< The process runs and answers.
    kExited,    ///< The process ended by itself (exitCode() tells how).
};

/// @brief Runs the game's server as a child process of the editor (`aylo-runtime --server`) and watches it through
/// its admin channel.
///
/// @details The admin port is a free local port, the token a random secret: both are made at each start() and given
/// to the child. The child's output goes to log() (and, with a prefix, to the sink given at construction: the editor's
/// Console). The child is stopped when the supervisor is destroyed, so no server outlives the editor. Not
/// thread-safe: call it from the editor's thread.
class RTYPE_EDITOR_API ServerSupervisor {
  public:
    using Clock = AdminSession::Clock;

    /// @param sink Where the lines of the server are copied, each prefixed with "[server] "; may be null.
    explicit ServerSupervisor(std::shared_ptr<console::ConsoleLog> sink = {});

    /// @brief Starts the server.
    /// @return Nothing, or why it could not start (already running, no runtime, bad options, the process failed).
    std::expected<void, std::string> start(ServerOptions options);

    /// @brief Stops the server: a stop request, then a kill after a short delay. Does nothing when stopped.
    void stop();

    /// @brief Stops the server and starts it again with the same options.
    std::expected<void, std::string> restart();

    /// @brief Reads the output, notices the end of the process and polls the admin channel. Call once a frame.
    void update(Clock::time_point now = Clock::now());

    /// @return What the server is doing.
    [[nodiscard]] ServerStatus status() const noexcept { return _status; }
    /// @return How the process ended, once the status is kExited.
    [[nodiscard]] std::optional<int> exitCode() const noexcept { return _exitCode; }
    /// @return The options of the last start().
    [[nodiscard]] const ServerOptions& options() const noexcept { return _options; }
    /// @return The admin port of the running server, 0 when there is none.
    [[nodiscard]] std::uint16_t adminPort() const noexcept { return _adminPort; }
    /// @return The admin channel of the running server, null while stopped or exited.
    [[nodiscard]] AdminSession* session() noexcept { return _session.get(); }
    /// @return The output of the server, kept for the Server panel.
    [[nodiscard]] console::ConsoleLog& log() noexcept { return _log; }

    /// @return A free local port, for the admin channel; 0 if none could be found.
    [[nodiscard]] static std::uint16_t findFreePort();
    /// @return A random secret of 32 hexadecimal digits.
    [[nodiscard]] static std::string makeToken();

  private:
    /// @brief Copies what the process wrote to the log and the sink.
    void drainOutput();
    /// @brief Adds a line of the editor's own to the log and the sink.
    void note(const std::string& text);

    std::shared_ptr<console::ConsoleLog> _sink;
    console::ConsoleLog _log;
    std::size_t _forwarded{0};  ///< Lines of _log already copied to the sink.
    ServerOptions _options;
    ServerStatus _status{ServerStatus::kStopped};
    std::optional<int> _exitCode;
    std::uint16_t _adminPort{0};
    std::optional<process::ChildProcess> _process;
    std::unique_ptr<AdminSession> _session;
};

}  // namespace rtype::editor::server
