/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Plugin
*/

#pragma once

#include <span>
#include <string_view>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs {
class App;

/// @brief A reusable bundle of components, resources, events and systems, added to an App.
/// @details build() only registers: it inserts resources, adds systems and other plugins. Work such as opening a
/// window or loading files belongs in Startup systems. Plugins are told apart by name: adding a plugin whose name was
/// already added does nothing. A plugin names the plugins it requires in getDependencies(): the App builds them first,
/// taking the missing ones from its PluginRegistry.
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

    /// @return The names of the plugins this one requires (their getName()), none by default. The App builds them
    /// before this one: those not added yet are built from its PluginRegistry, with their default settings.
    /// @details Return a static array, so the names outlive the call:
    /// @code
    /// static constexpr std::array<std::string_view, 1> kDependencies{TransformPlugin::kName};
    /// std::span<const std::string_view> getDependencies() const noexcept override { return kDependencies; }
    /// @endcode
    [[nodiscard]] virtual std::span<const std::string_view> getDependencies() const noexcept;

    /// @brief Registers what the plugin brings into the App.
    virtual void build(App& app) = 0;

    /// @brief Does what needs every plugin built first, once: reading data that names what other plugins registered
    /// (the components scripts declare, for instance). Does nothing by default.
    /// @details The App calls it on every plugin, in the order they were added, before its first frame
    /// (App::finishPlugins()); a plugin added after that is finished right after its build().
    virtual void finish(App& app);

  protected:
    Plugin() = default;
    Plugin(const Plugin& other) = default;
    Plugin& operator=(const Plugin& other) = default;
    Plugin(Plugin&& other) noexcept = default;
    Plugin& operator=(Plugin&& other) noexcept = default;
};
}  // namespace rtype::ecs
