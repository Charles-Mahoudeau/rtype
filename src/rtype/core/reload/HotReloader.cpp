/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HotReloader
*/

#include "HotReloader.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <ratio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ChangeSet.hpp"
#include "FileWatcher.hpp"
#include "ReloadOutcome.hpp"
#include "ReloadReport.hpp"
#include "Reloadable.hpp"

namespace rtype::core::reload {

namespace {
/// @return What @p reloadable made of @p changes; a failure if it threw.
ReloadOutcome runReload(Reloadable& reloadable, const ChangeSet& changes) noexcept {
    try {
        return reloadable.reload(changes);
    } catch (const std::exception& error) {
        return ReloadOutcome::failed(std::format("exception: {}", error.what()));
    } catch (...) {
        return ReloadOutcome::failed("unknown exception");
    }
}
}  // namespace

HotReloader::HotReloader(std::filesystem::path root, Sink sink)
    : _root{std::move(root)}, _sink{std::move(sink)}, _nextPoll{Clock::now() + kInterval} {}

void HotReloader::add(std::unique_ptr<Reloadable> reloadable) {
    std::vector<WatchedPath> watched = reloadable->watched();
    _entries.push_back(Entry{.reloadable = std::move(reloadable), .watcher = FileWatcher{_root, std::move(watched)}});
}

void HotReloader::clear() noexcept { _entries.clear(); }

std::size_t HotReloader::size() const noexcept { return _entries.size(); }

std::vector<ReloadReport> HotReloader::update(const Clock::time_point now) {
    if (now < _nextPoll) {
        return {};
    }
    _nextPoll = now + kInterval;
    return poll();
}

std::vector<ReloadReport> HotReloader::poll() {
    std::vector<ReloadReport> reports;
    for (auto& [reloadable, watcher] : _entries) {
        ChangeSet changes = watcher.poll();
        if (changes.empty()) {
            continue;
        }
        const Clock::time_point start = Clock::now();
        ReloadOutcome outcome = runReload(*reloadable, changes);
        const std::chrono::duration<double, std::milli> duration = Clock::now() - start;

        const ReloadReport& report = reports.emplace_back(ReloadReport{
            .source = std::string{reloadable->name()},
            .changes = std::move(changes),
            .outcome = std::move(outcome),
            .duration = duration,
        });
        log(report.format());
    }
    return reports;
}

void HotReloader::log(const std::string_view line) const {
    if (_sink) {
        _sink(line);
        return;
    }
    std::cerr << line << '\n';
}

}  // namespace rtype::core::reload
