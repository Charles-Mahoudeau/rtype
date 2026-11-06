/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderWatcher
*/

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

#include "plugins/shader/Export.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::shader {

class ShaderAssets;

/// @brief Follows the `assets/shaders` folder of a game, as the client resource "rtype.ShaderWatcher" the ShaderPlugin
/// inserts when hot reload is on: a file edited there is compiled again by ShaderAssets::reload().
///
/// @details It compares the files' modification times, which needs no system API and no dependency. Only the shaders
/// the game loaded are compiled: another file in the folder is ignored until the game asks for it. The folder may not
/// exist yet, and a file may disappear: nothing is reloaded until it comes back.
class RTYPE_PLUGIN_SHADER_API ShaderWatcher {
  public:
    using Clock = std::chrono::steady_clock;  ///< What update() is timed with.

    static constexpr std::string_view kDirectory = "assets/shaders";  ///< The folder, in the game directory.
    static constexpr std::chrono::milliseconds kInterval{500};        ///< Between two looks at the folder.

    /// @param gameDirectory The game directory; the files already in the folder are taken as they are.
    explicit ShaderWatcher(std::filesystem::path gameDirectory);
    ~ShaderWatcher() = default;
    ShaderWatcher(const ShaderWatcher& other) = delete;
    ShaderWatcher& operator=(const ShaderWatcher& other) = delete;
    ShaderWatcher(ShaderWatcher&& other) noexcept = default;
    ShaderWatcher& operator=(ShaderWatcher&& other) noexcept = default;

    /// @brief Calls poll() if kInterval went by since the last time.
    /// @param shaders The shaders to reload.
    /// @param now The current time.
    void update(ShaderAssets& shaders, Clock::time_point now);

    /// @brief Looks at the folder now, and reloads the shaders whose file changed or appeared.
    void poll(ShaderAssets& shaders);

  private:
    /// @brief When each file was last written, by its path in the game directory (normalizePath()).
    using WriteTimes = std::unordered_map<std::string, std::filesystem::file_time_type>;

    /// @return The files in the folder now; none if it does not exist.
    [[nodiscard]] WriteTimes scan() const;

    std::filesystem::path _gameDirectory;  ///< What the files' paths are relative to.
    WriteTimes _files;                     ///< The files, at the last look.
    Clock::time_point _nextPoll;           ///< When update() looks at the folder again.
};
RTYPE_ECS_RESOURCE(ShaderWatcher, "rtype.ShaderWatcher");

}  // namespace rtype::plugins::shader
