/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FileWatcher
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "ChangeSet.hpp"
#include "Reloadable.hpp"

namespace rtype::core::reload {

/// @brief Finds the files that changed between two looks, by their modification time and size.
///
/// @details It needs no system API and no dependency, so it behaves the same on every platform. A file is reported
/// only once it stayed the same for two looks in a row, so a save in progress (an editor writing in several steps) is
/// not read half written; a file that disappears is reported at once. The files that are there at construction are
/// taken as they are. A folder may not exist yet, and a file may come and go.
class FileWatcher {
  public:
    /// @param root What the watched paths and the reported paths are relative to.
    /// @param watched The files and folders to follow.
    FileWatcher(std::filesystem::path root, std::vector<WatchedPath> watched);
    ~FileWatcher() = default;
    FileWatcher(const FileWatcher& other) = default;
    FileWatcher& operator=(const FileWatcher& other) = default;
    FileWatcher(FileWatcher&& other) noexcept = default;
    FileWatcher& operator=(FileWatcher&& other) noexcept = default;

    /// @brief Looks at the files now.
    /// @return The files whose change settled since the last call.
    [[nodiscard]] ChangeSet poll();

  private:
    /// @brief What tells that a file changed.
    struct Stamp {
        std::filesystem::file_time_type time;  ///< When it was last written.
        std::uintmax_t size{0};                ///< How big it is.

        friend bool operator==(const Stamp& lhs, const Stamp& rhs) noexcept = default;
    };

    /// @brief The stamp of each file, by its path relative to the root.
    using Stamps = std::unordered_map<std::string, Stamp>;

    /// @return The files that exist now.
    [[nodiscard]] Stamps scan() const;

    /// @brief Records in @p stamps the regular file at @p path, by its path relative to @p root; a file that cannot be
    /// read is left out, as if it was not there.
    static void addFile(Stamps& stamps, const std::filesystem::path& root, const std::filesystem::path& path);

    std::filesystem::path _root;        ///< What the paths are relative to.
    std::vector<WatchedPath> _watched;  ///< What to follow.
    Stamps _confirmed;                  ///< The files as the last report left them.
    Stamps _pending;                    ///< The files that differ from _confirmed, seen once.
};

}  // namespace rtype::core::reload
