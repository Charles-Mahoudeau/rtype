/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleReloadable
*/

#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "plugins/capsule/Export.hpp"
#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"

namespace rtype::plugins::capsule {
class CapsuleFiles;
class Capsules;
struct ReloadedCapsules;

/// @brief Follows the capsule files of a game for the HotReloader: a `.capsule`, `.scene` or `.actor` file saved or
/// added under the assets is read again by CapsuleFiles::reload(), with the files that instance it, and the capsules
/// read go to ReloadedCapsules, so the CapsulePlugin reloads what was spawned from them in the next frame.
/// @details A file saved with a mistake keeps its old version, and the reload is reported as failed with the file, the
/// line and the column (the other files are applied). A removed file is ignored: its capsule stays as it was. The
/// CapsulePlugin adds one when hot reload is on.
class RTYPE_PLUGIN_CAPSULE_API CapsuleReloadable : public core::reload::Reloadable {
  public:
    /// @param files The game's capsule files; must outlive this.
    /// @param capsules What the files are read into; must outlive this.
    /// @param reloaded Gets the capsules read; must outlive this.
    /// @param assetDirectory The game's assets, relative to the HotReloader's root.
    CapsuleReloadable(CapsuleFiles* files, Capsules* capsules, ReloadedCapsules* reloaded,
                      std::filesystem::path assetDirectory);
    ~CapsuleReloadable() override = default;
    CapsuleReloadable(const CapsuleReloadable& other) = default;
    CapsuleReloadable& operator=(const CapsuleReloadable& other) = default;
    CapsuleReloadable(CapsuleReloadable&& other) noexcept = default;
    CapsuleReloadable& operator=(CapsuleReloadable&& other) noexcept = default;

    /// @return "capsules".
    [[nodiscard]] std::string_view name() const noexcept override;

    /// @return The capsule files of the assets, in every folder.
    [[nodiscard]] std::vector<core::reload::WatchedPath> watched() const override;

    /// @brief Reads again the files that changed or appeared, with those that instance them.
    [[nodiscard]] core::reload::ReloadOutcome reload(const core::reload::ChangeSet& changes) override;

  private:
    CapsuleFiles* _files{nullptr};         ///< The game's capsule files.
    Capsules* _capsules{nullptr};          ///< What they are read into.
    ReloadedCapsules* _reloaded{nullptr};  ///< What the next frame reloads.
    std::filesystem::path _assets;         ///< What the changed paths start with.
};
}  // namespace rtype::plugins::capsule
