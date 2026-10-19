/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** App
*/

#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginGroup.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief The entry point of a game: a world, its schedules, the plugins that filled them, and the loop that runs
/// them until a system sends AppExit.
/// @details One App runs one world in one run mode: a client, a dedicated server, or both sides in one process
/// (standalone). The same game plugins are added in every mode; each system's side decides where it runs. Every App
/// has a TimePlugin: if none was added, the App adds one with the defaults before its first frame. The App keeps a
/// pointer to its own world in its schedules, so it can be neither copied nor moved.
/// @code
/// rtype::ecs::App app{rtype::ecs::RunMode::kStandalone};
/// app.addPlugin(GamePlugin{});
/// app.addSystem(rtype::ecs::Phase::kUpdate, "game.camera", &follow).setSide(rtype::ecs::Side::kClient);
/// return app.run();
/// @endcode
class RTYPE_ECS_API App {
  public:
    /// @param mode What this process runs: it decides which systems run, from their side.
    explicit App(RunMode mode = RunMode::kStandalone);
    ~App();
    App(const App& other) = delete;
    App& operator=(const App& other) = delete;
    App(App&& other) = delete;
    App& operator=(App&& other) = delete;

    /// @return The world the systems run on.
    [[nodiscard]] World& getWorld() noexcept;
    [[nodiscard]] const World& getWorld() const noexcept;

    /// @return The world's schedules.
    [[nodiscard]] Schedules& getSchedules() noexcept;
    [[nodiscard]] const Schedules& getSchedules() const noexcept;

    /// @brief Adds a plugin and builds it at once; does nothing if a plugin with the same name was added.
    template <std::derived_from<Plugin> P>
    App& addPlugin(P plugin);

    /// @brief Adds a plugin and builds it at once; does nothing if a plugin with the same name was added.
    /// @details The plugin is recorded before it is built, so a plugin that adds itself again, directly or through
    /// another plugin, is not built twice. It is kept by the App until the App is destroyed.
    /// @param plugin The plugin; must not be null.
    App& addPlugin(std::unique_ptr<Plugin> plugin);

    /// @brief Adds a group of plugins (PluginGroup::build()).
    template <std::derived_from<PluginGroup> G>
    App& addPlugins(G group);

    /// @return Whether a plugin with this name was added.
    [[nodiscard]] bool hasPlugin(std::string_view name) const noexcept;

    /// @brief Makes a system of a function (or a lambda without captures) and adds it to a phase
    /// (Schedules::addSystem()).
    /// @throws exceptions::ScheduleException When a system of any phase already has this name.
    template <typename Function>
    Schedule::SystemConfig addSystem(Phase phase, std::string name, Function function);

    /// @brief Sets a resource of the world, replacing its value if any (Resources::insert()).
    template <Resource T>
    App& insertResource(T value);

    /// @brief Runs one frame (Schedules::runFrame()), adding a TimePlugin with the defaults first if none was added.
    /// @throws exceptions::ScheduleException When a schedule cannot be built.
    void runFrame();

    /// @brief Runs frames until a system sends AppExit, Startup first.
    /// @return The code of the first AppExit read.
    /// @throws exceptions::ScheduleException When a schedule cannot be built.
    int run();

  private:
    /// @return The code of the first AppExit sent since the last call, or nothing.
    [[nodiscard]] std::optional<int> takeExit() noexcept;

    World _world;                                   ///< The world the systems run on.
    Schedules _schedules;                           ///< The world's schedules; points to _world.
    std::vector<std::unique_ptr<Plugin>> _plugins;  ///< Every plugin added, in the order added.
    EventCursor _exitCursor;                        ///< The AppExit events read so far.
};
}  // namespace rtype::ecs

#include "App.tpp"
