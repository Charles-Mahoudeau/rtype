/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReloadReport
*/

#include "ReloadReport.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "ChangeSet.hpp"
#include "ReloadOutcome.hpp"

namespace rtype::core::reload {

namespace {
/// @return The items of @p items, separated by commas.
std::string join(const std::vector<std::string>& items) {
    std::string text;
    for (const std::string& item : items) {
        text += std::format("{}{}", text.empty() ? "" : ", ", item);
    }
    return text;
}

/// @brief Appends "<label> <file>, <file>" to @p text for the files of one kind, if there are any.
void appendFiles(std::string& text, const std::string_view label, const std::vector<std::string>& files) {
    if (!files.empty()) {
        text += std::format("{}{} {}", text.empty() ? "" : "; ", label, join(files));
    }
}

/// @return The files of @p changes, by kind.
std::string describeFiles(const ChangeSet& changes) {
    std::string text;
    appendFiles(text, "added", changes.added);
    appendFiles(text, "modified", changes.modified);
    appendFiles(text, "removed", changes.removed);
    return text;
}
}  // namespace

std::string ReloadReport::format() const {
    const std::string files = describeFiles(changes);

    switch (outcome.status) {
        case ReloadOutcome::Status::kApplied: {
            const std::string what = join(outcome.what);
            return std::format("[HotReload] {}: applied in {:.1f} ms ({}){}{}", source, duration.count(), files,
                               what.empty() ? "" : ": ", what);
        }
        case ReloadOutcome::Status::kFailed:
            return std::format("[HotReload] {}: failed after {:.1f} ms ({}), the previous version is kept: {}", source,
                               duration.count(), files, outcome.message);
        case ReloadOutcome::Status::kNeedsRebuild:
            return std::format("[HotReload] {}: cannot apply in place after {:.1f} ms ({}), the game is rebuilt: {}",
                               source, duration.count(), files, outcome.message);
    }
    return {};
}

}  // namespace rtype::core::reload
