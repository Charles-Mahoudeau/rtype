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
/// with a CapsuleInstance.
class RTYPE_PLUGIN_CAPSULE_API CapsulePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Capsule";  ///< The plugin's name.

    CapsulePlugin() = default;
    ~CapsulePlugin() override = default;
    CapsulePlugin(const CapsulePlugin& other) = default;
    CapsulePlugin& operator=(const CapsulePlugin& other) = default;
    CapsulePlugin(CapsulePlugin&& other) noexcept = default;
    CapsulePlugin& operator=(CapsulePlugin&& other) noexcept = default;

    /// @return "rtype.Capsule".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the Capsules resource if the world has none and sets its run mode to the App's, inserts the
    /// LoadedCapsule resource, and registers the CapsuleInstance component.
    void build(ecs::App& app) override;
};
}  // namespace rtype::plugins::capsule
