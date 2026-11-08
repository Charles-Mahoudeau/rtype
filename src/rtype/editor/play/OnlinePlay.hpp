/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** OnlinePlay
*/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <tl/expected.hpp>
#include <vector>

#include "editor/Export.hpp"
#include "editor/server/ServerSupervisor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

namespace rtype::editor::play {

/// @brief How Play runs the game: in the editor's own world (no network), or as a client of the server the editor runs.
enum class PlayMode : std::uint8_t {
    kStandalone,  ///< Both sides in one world, no network. The default.
    kOnline,      ///< The editor is a client of the child server (Server panel).
};

/// @brief Where Play online is.
enum class OnlineStage : std::uint8_t {
    kIdle,            ///< Not playing online: the editor is standalone.
    kStartingServer,  ///< Waiting for the child server to answer; the editor is still standalone.
    kConnecting,      ///< The editor was asked to rebuild as a client; the new App has not taken over yet.
    kOnline,          ///< The editor is a client of the server.
};

/// @brief The steps of "Play online", with no ImGui: start the server if it is not running, wait until it answers,
/// rebuild the editor's App as a client of it, and on Stop rebuild it standalone.
///
/// @details It outlives the Apps (it lives in the EditorSession), because every step but the first happens in another
/// App: the rebuild replaces the editor's world. The rebuild itself is a function the program gives (the Engine's
/// requestReload(mode, args)). Stop does not stop the server: the Server panel does, so the server, its clients and
/// the world it holds stay for the next Play. Not thread-safe: call it from the editor's thread.
class RTYPE_EDITOR_API OnlinePlay {
  public:
    /// @brief Rebuilds the editor's App in a mode with a command line.
    using Rebuild = std::function<void(ecs::RunMode, std::vector<std::string>)>;

    /// @brief Sets how to rebuild the App, and the command line to build it with: the flags of the program, to which
    /// the connection flags are added.
    void setRebuild(Rebuild rebuild, std::vector<std::string> baseArgs);

    /// @return What Play does.
    [[nodiscard]] PlayMode mode() const noexcept { return _mode; }
    /// @brief Chooses what Play does. Ignored while an online Play is under way.
    void setMode(PlayMode mode) noexcept;

    /// @return Where Play online is.
    [[nodiscard]] OnlineStage stage() const noexcept { return _stage; }
    /// @return Whether the editor is, or is about to be, a client of the server (no scene can be edited then).
    [[nodiscard]] bool isActive() const noexcept {
        return _stage == OnlineStage::kConnecting || _stage == OnlineStage::kOnline;
    }
    /// @return The last thing that went wrong, empty if nothing did.
    [[nodiscard]] const std::string& message() const noexcept { return _message; }

    /// @brief Starts Play online: starts the server with @p options unless it is running, then waits for it.
    /// @return Nothing, or why the server could not start (also kept in message()).
    tl::expected<void, std::string> begin(server::ServerSupervisor& supervisor, server::ServerOptions options);

    /// @brief Moves Play along; call it once a frame, after the supervisor's update(). Once the server answers, asks
    /// for the rebuild as a client. If the server stops while the editor waits for it, goes back to idle.
    void update(server::ServerSupervisor& supervisor);

    /// @brief Called by the editor of a rebuilt App: tells that the rebuild asked for happened.
    void adopt() noexcept;

    /// @brief Called by the editor of a rebuilt App that is not the client it was asked for (the engine went back to
    /// its previous mode because the client could not be built): gives up Play online, with @p reason as message().
    void fail(std::string reason);

    /// @brief Stops Play online: cancels the wait for the server, or rebuilds the editor standalone. The server keeps
    /// running. Does nothing when idle.
    void end();

  private:
    PlayMode _mode{PlayMode::kStandalone};
    OnlineStage _stage{OnlineStage::kIdle};
    Rebuild _rebuild;
    std::vector<std::string> _baseArgs;
    std::string _message;
};

}  // namespace rtype::editor::play
