/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Plugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs {
class App;

/// @brief A reusable bundle of components, resources, events and systems, added to an App.
/// @details build() only registers: it inserts resources, adds systems and other plugins. Work such as opening a
/// window or loading files belongs in Startup systems. Plugins are told apart by name: adding a plugin whose name was
/// already added does nothing, so plugins can add the plugins they depend on.
/// @code
/// class GamePlugin : public rtype::ecs::Plugin {
///   public:
///     std::string_view getName() const noexcept override { return "game"; }
///     void build(rtype::ecs::App& app) override {
///         app.addSystem(rtype::ecs::Phase::kFixedUpdate, "game.movement", &movement);
///     }
/// };
/// app.addPlugin(GamePlugin{});
/// @endcode
class RTYPE_ECS_API Plugin {
  public:
    virtual ~Plugin();

    /// @return The plugin's name, unique in an App ("rtype.time", "game").
    [[nodiscard]] virtual std::string_view getName() const noexcept = 0;

    /// @brief Registers what the plugin brings into the App.
    virtual void build(App& app) = 0;

  protected:
    Plugin() = default;
    Plugin(const Plugin& other) = default;
    Plugin& operator=(const Plugin& other) = default;
    Plugin(Plugin&& other) noexcept = default;
    Plugin& operator=(Plugin&& other) noexcept = default;
};
}  // namespace rtype::ecs
