/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptingPlugin
*/

#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::scripting {
/// @brief Runs the Luau files of a game on the App: the components and systems they declare join it.
///
/// @details build() creates a ScriptHost with a modules::ScriptResolver over the scripts directory, then requires the
/// entry points: `main` first, then `server` if the process runs the server side, then `client` if it runs the client
/// side (a standalone process runs both, so all three). Only the entries and what they `require` run (see
/// `manual/pages/modules.md`); a module several of them require runs once. With no entry set, `main` defaults to
/// `main` (`scripts/main.luau`) and a missing default entry logs a warning on `std::cerr` and loads nothing; an entry
/// that was set and is missing fails the build, naming its setting. A script's chunk name, which names it in errors and
/// owns what it declares, is `scripts/<path in the directory>`. Each file has its own globals (see ScriptHost), and a
/// file nobody requires is ignored, even by a reload. The host lives as long as the plugin, that is as long as the
/// App; Config::onBuilt gives it to the program.
/// @code
/// app.addPlugin(rtype::scripting::ScriptingPlugin{{.options = {.isClient = true, .isServer = true}}});
/// @endcode
class RTYPE_SCRIPTING_API ScriptingPlugin : public ecs::Plugin {
  public:
    /// @brief What the plugin loads and the flags the scripts see.
    struct Config {
        std::filesystem::path directory{"game/scripts"};  ///< Where the scripts are.
        std::optional<std::string> main;    ///< `scripts.main`: the entry every process runs first, no extension.
        std::optional<std::string> server;  ///< `scripts.server`: the entry of the server side.
        std::optional<std::string> client;  ///< `scripts.client`: the entry of the client side.
        std::map<std::string, std::string> aliases;  ///< Extra `require` aliases: name, then a folder in scripts/.
        ScriptHost::Options options;                 ///< The flags of `engine`.

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

    /// @brief Creates the host and runs the entry scripts.
    /// @throws exceptions::ScriptLoadException When an entry that was set is missing or invalid, or a script fails.
    void build(ecs::App& app) override;

  private:
    Config _config;                     ///< The scripts directory and the flags of `engine`.
    std::unique_ptr<ScriptHost> _host;  ///< The runtime of the scripts; null until build().
};
}  // namespace rtype::scripting
