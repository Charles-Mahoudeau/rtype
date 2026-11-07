/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsulePlugin
*/

#pragma once

#include <string_view>

#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::capsule {
/// @brief Gives the world its capsules: inserts the Capsules resource ("rtype.Capsules"), spawning on the App's side.
/// @details Requires no plugin, so a server adds it alone. A game lists it in `project.luau` as "rtype.Capsule".
/// Capsules are spawned, loaded (a switch: LoadedCapsule) and reloaded with CapsuleSpawn.hpp, each spawned root marked
/// with a CapsuleInstance. In a game (an asset directory), finish() reads every capsule file under the assets
/// (CapsuleFiles), once the other plugins, scripts included, have registered their components. A Render system,
/// "rtype.Capsule.HotReload" on every side, reloads what was spawned from the capsules the ReloadedCapsules resource
/// names. With hot reload (the HotReload resource), it adds a CapsuleReloadable to the engine's reloader, which reads
/// again the files saved since and names them there; a tool that redefines a capsule names it there too.
class RTYPE_PLUGIN_CAPSULE_API CapsulePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Capsule";                       ///< The plugin's name.
    static constexpr std::string_view kHotReloadSystem = "rtype.Capsule.HotReload";  ///< Respawns changed capsules.

    CapsulePlugin() = default;
    ~CapsulePlugin() override = default;
    CapsulePlugin(const CapsulePlugin& other) = default;
    CapsulePlugin& operator=(const CapsulePlugin& other) = default;
    CapsulePlugin(CapsulePlugin&& other) noexcept = default;
    CapsulePlugin& operator=(CapsulePlugin&& other) noexcept = default;

    /// @return "rtype.Capsule".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the Capsules resource if the world has none and sets its run mode to the App's, inserts the
    /// LoadedCapsule and ReloadedCapsules resources and the reload system, registers the CapsuleInstance component and
    /// the engine calls (addCapsuleCalls()). In a game, inserts CapsuleFiles and, with hot reload, the
    /// CapsuleReloadable.
    void build(ecs::App& app) override;

    /// @brief Reads the game's capsule files (CapsuleFiles::loadAll()), if it has an asset directory.
    /// @throws exceptions::CapsuleException When a file cannot be read.
    void finish(ecs::App& app) override;
};
}  // namespace rtype::plugins::capsule
