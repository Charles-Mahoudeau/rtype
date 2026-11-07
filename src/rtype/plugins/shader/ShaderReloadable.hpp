/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderReloadable
*/

#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "plugins/shader/Export.hpp"
#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/Reloadable.hpp"

namespace rtype::plugins::shader {

class ShaderAssets;

/// @brief Follows the shaders of a game for the HotReloader: a file edited in `<assets>/shaders` is compiled again by
/// ShaderAssets::reload(), and the ShaderPlugin hands the new shader to the renderer in the next frame.
///
/// @details Only the shaders the game loaded are compiled: another file in the folder is ignored until the game asks
/// for it. A shader that does not compile keeps its last version, and the reload is reported as failed with the
/// reason in the log. The ShaderPlugin adds one when hot reload is on.
class RTYPE_PLUGIN_SHADER_API ShaderReloadable : public core::reload::Reloadable {
  public:
    static constexpr std::string_view kDirectory = "shaders";  ///< The folder, in the assets' directory.

    /// @param shaders The shaders to reload; must outlive this.
    /// @param assetDirectory The game's assets, relative to the HotReloader's root.
    ShaderReloadable(ShaderAssets* shaders, std::filesystem::path assetDirectory);
    ~ShaderReloadable() override = default;
    ShaderReloadable(const ShaderReloadable& other) = default;
    ShaderReloadable& operator=(const ShaderReloadable& other) = default;
    ShaderReloadable(ShaderReloadable&& other) noexcept = default;
    ShaderReloadable& operator=(ShaderReloadable&& other) noexcept = default;

    /// @return "shaders".
    [[nodiscard]] std::string_view name() const noexcept override;

    /// @return The shaders folder of the game.
    [[nodiscard]] std::vector<core::reload::WatchedPath> watched() const override;

    /// @brief Compiles again the shaders whose file changed or appeared.
    [[nodiscard]] core::reload::ReloadOutcome reload(const core::reload::ChangeSet& changes) override;

  private:
    ShaderAssets* _shaders{nullptr};  ///< The game's shaders.
    std::filesystem::path _assets;    ///< What the changed paths start with.
};

}  // namespace rtype::plugins::shader
