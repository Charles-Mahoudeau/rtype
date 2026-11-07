/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ProjectReloadable
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

namespace rtype::engine::reload {

/// @brief Follows `project.luau`: a change to the settings cannot be applied in place (the window, the renderer, the
/// network and the plugins come from them), so it asks for the app to be built again, once the new project is known to
/// be valid.
class RTYPE_ENGINE_API ProjectReloadable final : public core::reload::Reloadable {
  public:
    /// @brief Throws what is wrong with the project: the file does not run, or a setting is invalid.
    using Validate = std::function<void()>;

    /// @param file The project file, relative to the game directory.
    /// @param validate Reads the project again without building anything.
    ProjectReloadable(std::filesystem::path file, Validate validate);
    ~ProjectReloadable() override = default;
    ProjectReloadable(const ProjectReloadable& other) = default;
    ProjectReloadable& operator=(const ProjectReloadable& other) = default;
    ProjectReloadable(ProjectReloadable&& other) noexcept = default;
    ProjectReloadable& operator=(ProjectReloadable&& other) noexcept = default;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::vector<core::reload::WatchedPath> watched() const override;

    /// @return `needsRebuild` if the new project is valid; `failed` with why not, and the game goes on with the old
    /// one.
    [[nodiscard]] core::reload::ReloadOutcome reload(const core::reload::ChangeSet& changes) override;

  private:
    std::filesystem::path _file;  ///< The project file, relative to the game directory.
    Validate _validate;           ///< Reads the project again.
};

}  // namespace rtype::engine::reload
