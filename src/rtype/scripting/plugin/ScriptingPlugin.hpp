/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptingPlugin
*/

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::scripting {
/// @brief Runs the Luau files of a game on the App: the components and systems they declare join it.
///
/// @details build() creates a ScriptHost, then loads every `.luau` file found under the scripts directory, in sorted
/// path order, so the order is the same on every machine. A missing directory loads nothing and logs a warning on
/// `std::cerr`. The same files load on every side (server and clients): a script tells the sides apart with
/// `engine.isClient` and `engine.isServer`. The host lives as long as the plugin, that is as long as the App;
/// Config::onBuilt gives it to the program.
/// @code
/// app.addPlugin(rtype::scripting::ScriptingPlugin{{.options = {.isClient = true, .isServer = true}}});
/// @endcode
class RTYPE_SCRIPTING_API ScriptingPlugin : public ecs::Plugin {
  public:
    /// @brief What the plugin loads and the flags the scripts see.
    struct Config {
        std::filesystem::path directory{"game/scripts"};  ///< Where the scripts are, searched recursively.
        ScriptHost::Options options;                      ///< The flags of `engine`.

        /// @brief Called with the host once the scripts ran, so the program can reload them later (see
        /// ScriptHost::reloadScript()). The host lives as long as the plugin, that is as long as the App. Optional.
        std::function<void(ScriptHost&)> onBuilt;
    };

    /// @param config The scripts directory and the flags of `engine`.
    explicit ScriptingPlugin(Config config);
    ~ScriptingPlugin() override;
    ScriptingPlugin(const ScriptingPlugin& other) = delete;
    ScriptingPlugin& operator=(const ScriptingPlugin& other) = delete;
    ScriptingPlugin(ScriptingPlugin&& other) noexcept;
    ScriptingPlugin& operator=(ScriptingPlugin&& other) noexcept;

    /// @return "rtype.scripting".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Creates the host and runs the scripts.
    /// @throws exceptions::ScriptLoadException When the directory cannot be read, a file cannot be read, or a script
    /// fails.
    void build(ecs::App& app) override;

  private:
    Config _config;                     ///< The scripts directory and the flags of `engine`.
    std::unique_ptr<ScriptHost> _host;  ///< The runtime of the scripts; null until build().
};
}  // namespace rtype::scripting
