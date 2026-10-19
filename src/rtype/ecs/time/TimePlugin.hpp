/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TimePlugin
*/

#pragma once

#include <chrono>
#include <optional>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"

namespace rtype::ecs {
/// @brief Time for an App: inserts the Time resource and installs the frame clock (TimeClock) that measures each
/// frame, runs FixedUpdate at a fixed rate and keeps Time up to date.
/// @details Every App has one: if none was added, the App adds one with the defaults (60 ticks per second, at most 5
/// per frame) before its first frame; MinimalPlugins adds the same default. To configure it, add your own before
/// MinimalPlugins and before the first frame: the default one is then skipped, by name. Frames are measured with
/// std::chrono::steady_clock; manual() makes every frame last a fixed duration instead, for deterministic tests and
/// replays. A clock installed directly on the schedules is replaced by the default plugin's on the first frame: a
/// custom clock comes from a plugin named "rtype.time" added in place of this one.
/// @code
/// app.addPlugin(rtype::ecs::TimePlugin{rtype::ecs::FixedTimestep{30, 5}});                    // 30 Hz
/// app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::milliseconds{16}));               // tests
/// @endcode
class RTYPE_ECS_API TimePlugin : public Plugin {
  public:
    static constexpr std::string_view kName = "rtype.time";  ///< The plugin's name.

    /// @return A time plugin whose frames all last `frame`.
    [[nodiscard]] static TimePlugin manual(std::chrono::nanoseconds frame,
                                           FixedTimestep timestep = FixedTimestep{}) noexcept;

    /// @param timestep The fixed rate and the most fixed ticks per frame.
    explicit TimePlugin(FixedTimestep timestep = FixedTimestep{}) noexcept;
    ~TimePlugin() override = default;
    TimePlugin(const TimePlugin& other) noexcept = default;
    TimePlugin& operator=(const TimePlugin& other) noexcept = default;
    TimePlugin(TimePlugin&& other) noexcept = default;
    TimePlugin& operator=(TimePlugin&& other) noexcept = default;

    /// @return The fixed timestep the frames run.
    [[nodiscard]] const FixedTimestep& getFixedTimestep() const noexcept;

    /// @return The duration of every frame, or nothing when the real clock is measured.
    [[nodiscard]] std::optional<std::chrono::nanoseconds> getManualFrame() const noexcept;

    /// @return "rtype.time".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts Time (its fixedDelta set) and installs the TimeClock, replacing any clock.
    void build(App& app) override;

  private:
    FixedTimestep _timestep;                               ///< The fixed rate and the most ticks per frame.
    std::optional<std::chrono::nanoseconds> _manualFrame;  ///< Every frame's duration, if manual.
};
}  // namespace rtype::ecs
