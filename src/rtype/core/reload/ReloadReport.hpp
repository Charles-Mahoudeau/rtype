/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReloadReport
*/

#pragma once

#include <chrono>
#include <ratio>
#include <string>

#include "ChangeSet.hpp"
#include "ReloadOutcome.hpp"

namespace rtype::core::reload {

/// @brief What a HotReloader did for one Reloadable: why it ran, what it did, and how long it took.
struct ReloadReport {
    std::string source;                                    ///< The name of the Reloadable.
    ChangeSet changes;                                     ///< The files that made it run.
    ReloadOutcome outcome;                                 ///< How it ended.
    std::chrono::duration<double, std::milli> duration{};  ///< How long reload() took.

    /// @return Whether only a rebuild of the app applies these changes.
    [[nodiscard]] bool needsRebuild() const noexcept { return outcome.status == ReloadOutcome::Status::kNeedsRebuild; }

    /// @return The line the log gets, without the end of line, for instance
    /// `[HotReload] scripts: applied in 3.2 ms (modified scripts/ship.luau): system ship.move, component Ship`.
    [[nodiscard]] std::string format() const;
};

}  // namespace rtype::core::reload
