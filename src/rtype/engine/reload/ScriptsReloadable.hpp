/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptsReloadable
*/

#pragma once

#include <filesystem>
#include <functional>
#include <string_view>
#include <vector>

#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"
#include "rtype/engine/Export.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::engine::reload {

/// @brief Follows the `.luau` files under `scripts/`: a change set is reloaded as one batch by the ScriptHost: the
/// changed files run again with the files that require them, and the systems and components are swapped in the live
/// world; a deleted file loses its systems.
class RTYPE_ENGINE_API ScriptsReloadable final : public core::reload::Reloadable {
  public:
    /// @brief Gives the host of the app being run, or null while there is none.
    using HostGetter = std::function<scripting::ScriptHost*()>;

    /// @param gameDirectory The game directory, absolute: the scripts are known to the host by their path in it.
    /// @param scriptsDirectory The scripts, relative to the game directory.
    /// @param host Gives the host to reload the scripts on.
    ScriptsReloadable(std::filesystem::path gameDirectory, std::filesystem::path scriptsDirectory, HostGetter host);
    ~ScriptsReloadable() override = default;
    ScriptsReloadable(const ScriptsReloadable& other) = default;
    ScriptsReloadable& operator=(const ScriptsReloadable& other) = default;
    ScriptsReloadable(ScriptsReloadable&& other) noexcept = default;
    ScriptsReloadable& operator=(ScriptsReloadable&& other) noexcept = default;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::vector<core::reload::WatchedPath> watched() const override;

    /// @return `applied` if the batch was swapped; `needsRebuild` if it cannot be in place, or there is no app to
    /// reload on; otherwise `failed` (a file that cannot be read or a script error), and the old code keeps running.
    [[nodiscard]] core::reload::ReloadOutcome reload(const core::reload::ChangeSet& changes) override;

  private:
    std::filesystem::path _gameDirectory;     ///< The game directory.
    std::filesystem::path _scriptsDirectory;  ///< The scripts, relative to it.
    HostGetter _host;                         ///< The host of the app being run.
};

}  // namespace rtype::engine::reload
