/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginGroup
*/

#pragma once

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs {
class App;

/// @brief A set of plugins added together (MinimalPlugins, and later DefaultPlugins).
/// @details build() adds each plugin through the App, so a plugin already added is skipped as usual.
/// @code
/// class GamePlugins : public rtype::ecs::PluginGroup {
///   public:
///     void build(rtype::ecs::App& app) override { app.addPlugin(WeaponsPlugin{}).addPlugin(EnemiesPlugin{}); }
/// };
/// app.addPlugins(GamePlugins{});
/// @endcode
class RTYPE_ECS_API PluginGroup {
  public:
    virtual ~PluginGroup();

    /// @brief Adds the group's plugins to the App.
    virtual void build(App& app) = 0;

  protected:
    PluginGroup() = default;
    PluginGroup(const PluginGroup& other) = default;
    PluginGroup& operator=(const PluginGroup& other) = default;
    PluginGroup(PluginGroup&& other) noexcept = default;
    PluginGroup& operator=(PluginGroup&& other) noexcept = default;
};
}  // namespace rtype::ecs
