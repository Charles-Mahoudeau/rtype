/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReloadOutcome
*/

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace rtype::core::reload {

/// @brief What a Reloadable did with a ChangeSet.
struct ReloadOutcome {
    /// @brief How the reload ended.
    enum class Status : std::uint8_t {
        kApplied,       ///< The new version is live.
        kFailed,        ///< The new version is wrong (it does not compile, it raised...): the old one is kept.
        kNeedsRebuild,  ///< It cannot be applied in place: the whole app has to be built again.
    };

    Status status{Status::kApplied};  ///< How it ended.
    std::string message;              ///< Why it failed, or why it needs a rebuild; empty when applied.
    std::vector<std::string> what;    ///< What changed, short ("system ship.move", "shader flash.glsl").

    /// @return An applied reload that changed @p what.
    [[nodiscard]] static ReloadOutcome applied(std::vector<std::string> what = {}) {
        return {.status = Status::kApplied, .message = {}, .what = std::move(what)};
    }

    /// @return A failed reload, which kept the old version.
    [[nodiscard]] static ReloadOutcome failed(std::string message) {
        return {.status = Status::kFailed, .message = std::move(message), .what = {}};
    }

    /// @return A reload that only a rebuild of the app can apply.
    [[nodiscard]] static ReloadOutcome needsRebuild(std::string reason) {
        return {.status = Status::kNeedsRebuild, .message = std::move(reason), .what = {}};
    }
};

}  // namespace rtype::core::reload
