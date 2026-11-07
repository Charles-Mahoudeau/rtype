/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HotReloader
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "FileWatcher.hpp"
#include "ReloadReport.hpp"
#include "Reloadable.hpp"

namespace rtype::core::reload {

/// @brief Watches the files of several Reloadable features and tells each one what changed.
///
/// @details It knows no feature: every one is a Reloadable it was given. update() looks at the files at most once per
/// kInterval; for each feature whose files changed it calls Reloadable::reload(), times it, logs a ReloadReport (what
/// ran, why, which files, how long) and returns it, so the caller can act on a rebuild. An exception thrown by a
/// feature is caught and reported as a failure: a reload never takes the game down. It keeps no global state.
class HotReloader {
  public:
    using Clock = std::chrono::steady_clock;             ///< What update() is timed with.
    using Sink = std::function<void(std::string_view)>;  ///< Where a log line goes, without its end of line.

    static constexpr std::chrono::milliseconds kInterval{250};  ///< Between two looks at the files.

    /// @param root What the watched paths are relative to (the game directory).
    /// @param sink Where the log goes; null: the standard error.
    explicit HotReloader(std::filesystem::path root, Sink sink = nullptr);
    ~HotReloader() = default;
    HotReloader(const HotReloader& other) = delete;
    HotReloader& operator=(const HotReloader& other) = delete;
    HotReloader(HotReloader&& other) noexcept = default;
    HotReloader& operator=(HotReloader&& other) noexcept = default;

    /// @brief Starts watching the files of a feature, as they are now.
    void add(std::unique_ptr<Reloadable> reloadable);

    /// @brief Forgets every feature.
    void clear() noexcept;

    /// @return What the watched paths are relative to (the game directory).
    [[nodiscard]] const std::filesystem::path& root() const noexcept { return _root; }

    /// @return How many features are watched.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Calls poll() if kInterval went by since the last time.
    /// @param now The current time.
    /// @return The reports of the reloads it made; none most of the time.
    [[nodiscard]] std::vector<ReloadReport> update(Clock::time_point now);

    /// @brief Looks at the files now, and reloads the features whose files changed.
    /// @return The reports of the reloads it made.
    [[nodiscard]] std::vector<ReloadReport> poll();

  private:
    /// @brief A feature and the files it follows.
    struct Entry {
        std::unique_ptr<Reloadable> reloadable;  ///< The feature.
        FileWatcher watcher;                     ///< Its files.
    };

    /// @brief Sends a line to the sink.
    void log(std::string_view line) const;

    std::filesystem::path _root;  ///< What the watched paths are relative to.
    Sink _sink;                   ///< Where the log goes.
    std::vector<Entry> _entries;  ///< The features, in the order they were added.
    Clock::time_point _nextPoll;  ///< When update() looks at the files again.
};

}  // namespace rtype::core::reload
