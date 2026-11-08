/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ChildProcess
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

namespace rtype::editor::process {

/// @brief What to run: a program, its arguments and the folder it starts in.
struct ProcessSpec {
    /// The executable. A bare name is looked up in the PATH.
    std::filesystem::path program;
    /// The arguments, without the program itself.
    std::vector<std::string> args;
    /// The folder the process starts in; empty keeps the editor's.
    std::filesystem::path workingDirectory;
};

/// @brief A process started by the editor, that the editor watches and always ends.
///
/// Its standard output and error are piped into the editor (drainOutput()), its standard input is closed. The
/// destructor stops it (a stop request, then a kill after a short delay), so no child outlives the editor. Not
/// thread-safe: use it from one thread.
class RTYPE_EDITOR_API ChildProcess {
  public:
    /// @brief Starts a process.
    /// @return The process, or why it could not start (no such program, no permission, no more processes).
    [[nodiscard]] static std::expected<ChildProcess, std::string> spawn(const ProcessSpec& spec);

    ChildProcess(ChildProcess&& other) noexcept;
    ChildProcess& operator=(ChildProcess&& other) noexcept;
    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;
    ~ChildProcess();

    /// @return The operating system's id of the process.
    [[nodiscard]] std::uint64_t pid() const;

    /// @return Whether the process still runs. Notices an exit; never blocks.
    [[nodiscard]] bool isRunning();

    /// @return The exit code once the process ended (a negative number when a signal ended it), nothing while it runs.
    [[nodiscard]] std::optional<int> exitCode();

    /// @brief Asks the process to end by itself (SIGTERM; on Windows it can only be ended, as stop()'s kill).
    void requestStop();

    /// @brief Ends the process at once and waits for it.
    void kill();

    /// @brief Asks the process to end, waits up to @p grace, then kills it.
    void stop(std::chrono::milliseconds grace = std::chrono::milliseconds{1500});

    /// @brief Waits up to @p timeout for the process to end.
    /// @return Whether it ended.
    [[nodiscard]] bool waitFor(std::chrono::milliseconds timeout);

    /// @return What the process wrote to its output and error since the last call. Never blocks.
    [[nodiscard]] std::string drainOutput();

  private:
    class Impl;
    explicit ChildProcess(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> _impl;
};

}  // namespace rtype::editor::process
