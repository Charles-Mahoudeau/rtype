/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientLauncher
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <vector>

#include "editor/Export.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/process/ChildProcess.hpp"

namespace rtype::editor::process {

/// @brief Where the detached clients connect and what runs them.
struct ClientOptions {
    std::filesystem::path gameDirectory;  ///< The game folder.
    std::string host{"127.0.0.1"};        ///< The server's host.
    std::uint16_t port{4242};             ///< The server's port.
    std::filesystem::path runtime;        ///< The `aylo-runtime` to run; empty: the one next to the editor.
};

/// @brief What the panel shows of one client.
struct ClientInfo {
    int number{0};                  ///< 1, 2, 3... for as long as the launcher lives, never reused.
    std::uint64_t pid{0};           ///< The process id.
    bool running{false};            ///< Whether the process still runs.
    std::optional<int> exitCode;    ///< How it ended, once it did.
    std::vector<std::string> args;  ///< The extra flags it was launched with.
};

/// @brief Launches game clients as detached windows (`aylo-runtime <game> --host <h> --port <p>`) and keeps them in a
/// list, to see, kill and start again together.
///
/// @details Their output goes to the sink given at construction, each line prefixed with "[client N] ". They are all
/// stopped when the launcher is destroyed, so none outlives the editor. Not thread-safe.
class RTYPE_EDITOR_API ClientLauncher {
  public:
    /// @param sink Where the lines of the clients are copied; may be null.
    explicit ClientLauncher(std::shared_ptr<console::ConsoleLog> sink = {});
    ~ClientLauncher();
    ClientLauncher(const ClientLauncher&) = delete;
    ClientLauncher& operator=(const ClientLauncher&) = delete;
    ClientLauncher(ClientLauncher&&) = delete;
    ClientLauncher& operator=(ClientLauncher&&) = delete;

    /// @brief Starts one client.
    /// @param options Where it connects.
    /// @param extraArgs More flags for this client alone (`--name p2`).
    /// @return Its number, or why it could not start.
    tl::expected<int, std::string> launch(const ClientOptions& options, const std::vector<std::string>& extraArgs);

    /// @brief Stops client @p number (it stays in the list, ended). Does nothing for an unknown number.
    void kill(int number);

    /// @brief Stops every client, together: one stop request for all, then one wait, then the kill of the rest.
    void killAll();

    /// @brief Stops every client and starts each again, with its flags, under its number.
    /// @return Nothing, or the first error.
    tl::expected<void, std::string> restartAll(const ClientOptions& options);

    /// @brief Removes the clients that ended from the list.
    void removeEnded();

    /// @brief Reads the output and notices the clients that ended. Call once a frame.
    void update();

    /// @return The clients, by number.
    [[nodiscard]] std::vector<ClientInfo> list() const;

    /// @return How many clients run.
    [[nodiscard]] std::size_t runningCount() const noexcept;

  private:
    /// @brief A client of the list.
    struct Client {
        int number{0};
        std::vector<std::string> args;
        std::optional<ChildProcess> process;  ///< Empty once ended and reaped.
        std::optional<int> exitCode;
        std::unique_ptr<console::ConsoleLog> log;  ///< Cuts its output into whole lines.
        std::size_t forwarded{0};                  ///< Lines of `log` already copied to the sink.
    };

    /// @brief Starts the process of a client.
    static tl::expected<ChildProcess, std::string> spawn(const ClientOptions& options,
                                                         const std::vector<std::string>& extraArgs);
    /// @brief Copies what the process of @p client wrote to its log and the sink.
    void drain(Client& client);
    /// @brief Adds a line of the editor's own about @p client to its log and the sink.
    void note(Client& client, const std::string& text);

    std::shared_ptr<console::ConsoleLog> _sink;
    std::vector<Client> _clients;
    int _next{1};
};

}  // namespace rtype::editor::process
