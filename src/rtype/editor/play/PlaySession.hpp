/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlaySession
*/

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "editor/Export.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::editor::play {

/// @brief Whether the game is being played in the editor, and what the world held when it started, so that Stop puts
/// the scene back. No ImGui here: the toolbar calls these.
/// @details In the editor, the simulation is paused while editing (ecs::Schedules::setPaused()): the scene shows, the
/// gameplay does not move. Play runs it; Pause and Step only exist while playing. Stop pauses it again, despawns every
/// entity spawned since Play, and respawns the open scene from its capsule, so its values are those of the file being
/// edited. Entities that were there before Play and are not from the scene keep what the game did to them, and the
/// resources and the scripts' state are not put back.
class RTYPE_EDITOR_API PlaySession {
  public:
    PlaySession() = default;
    ~PlaySession() = default;
    PlaySession(const PlaySession& other) = default;
    PlaySession& operator=(const PlaySession& other) = default;
    PlaySession(PlaySession&& other) noexcept = default;
    PlaySession& operator=(PlaySession&& other) noexcept = default;

    /// @return Whether the game is being played (between play() and stop()), paused or not.
    [[nodiscard]] bool isPlaying() const noexcept;

    /// @brief Pauses @p app's simulation: the editor starts editing.
    static void edit(ecs::App& app) noexcept;

    /// @brief Remembers the entities of @p app's world and runs the simulation. Does nothing while playing.
    void play(ecs::App& app);

    /// @brief While playing, pauses or resumes the simulation.
    void togglePause(ecs::App& app) const noexcept;

    /// @brief While playing and paused, runs the next frame whole (ecs::Schedules::step()).
    void step(ecs::App& app) const noexcept;

    /// @brief Pauses the simulation, despawns the entities spawned since play(), and respawns @p scene, the capsule
    /// being edited, if any (shows it again if its root is gone). Does nothing unless playing.
    /// @return Why the scene could not be shown again; nothing when it was, or when there is none.
    std::optional<std::string> stop(ecs::App& app, std::optional<plugins::capsule::CapsuleId> scene);

  private:
    std::vector<ecs::Entity> _before;  ///< The entities alive at play(), sorted by their bits.
    bool _playing{false};              ///< Between play() and stop().
};

}  // namespace rtype::editor::play
