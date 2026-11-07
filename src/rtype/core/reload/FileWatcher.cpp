/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FileWatcher
*/

#include "FileWatcher.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "ChangeSet.hpp"
#include "Reloadable.hpp"
#include "core/asset/AssetFiles.hpp"

namespace rtype::core::reload {

namespace {
/// @return Whether @p path has one of @p extensions; every path has one when the list is empty.
bool hasExtension(const std::filesystem::path& path, const std::vector<std::string>& extensions) {
    return extensions.empty() || std::ranges::find(extensions, path.extension().string()) != extensions.end();
}
}  // namespace

FileWatcher::FileWatcher(std::filesystem::path root, std::vector<WatchedPath> watched)
    : _root{std::move(root)}, _watched{std::move(watched)}, _confirmed{scan()} {}

ChangeSet FileWatcher::poll() {
    Stamps current = scan();
    ChangeSet changes;

    for (const auto& [path, stamp] : current) {
        const auto known = _confirmed.find(path);
        if (known != _confirmed.end() && known->second == stamp) {
            _pending.erase(path);
            continue;
        }
        const auto waiting = _pending.find(path);
        if (waiting == _pending.end() || !(waiting->second == stamp)) {
            _pending.insert_or_assign(path, stamp);
            continue;
        }
        // The same stamp twice in a row: whoever wrote it is done.
        (known == _confirmed.end() ? changes.added : changes.modified).push_back(path);
        _pending.erase(waiting);
        _confirmed.insert_or_assign(path, stamp);
    }
    for (auto it = _confirmed.begin(); it != _confirmed.end();) {
        if (current.contains(it->first)) {
            ++it;
            continue;
        }
        changes.removed.push_back(it->first);
        _pending.erase(it->first);
        it = _confirmed.erase(it);
    }
    std::erase_if(_pending, [&current](const auto& entry) { return !current.contains(entry.first); });

    std::ranges::sort(changes.added);
    std::ranges::sort(changes.modified);
    std::ranges::sort(changes.removed);
    return changes;
}

FileWatcher::Stamps FileWatcher::scan() const {
    Stamps stamps;
    for (const WatchedPath& watch : _watched) {
        const std::filesystem::path fullPath = _root / watch.path;
        std::error_code error;
        if (std::filesystem::is_directory(fullPath, error)) {
            std::filesystem::recursive_directory_iterator entry{
                fullPath, std::filesystem::directory_options::skip_permission_denied, error};
            const std::filesystem::recursive_directory_iterator end;
            while (!error && entry != end) {
                if (hasExtension(entry->path(), watch.extensions)) {
                    addFile(stamps, _root, entry->path());
                }
                entry.increment(error);
            }
        } else {
            addFile(stamps, _root, fullPath);
        }
    }
    return stamps;
}

void FileWatcher::addFile(Stamps& stamps, const std::filesystem::path& root, const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        return;
    }
    const auto time = std::filesystem::last_write_time(path, error);
    if (error) {
        return;
    }
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    if (error) {
        return;
    }
    stamps.insert_or_assign(asset::normalizePath(path.lexically_relative(root)), Stamp{.time = time, .size = size});
}

}  // namespace rtype::core::reload
