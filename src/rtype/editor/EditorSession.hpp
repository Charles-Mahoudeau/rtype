/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorSession
*/

#pragma once

#include <memory>
#include <string>

#include "editor/Export.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/console/OutputCapture.hpp"
#include "editor/console/ScriptPrompt.hpp"
#include "editor/play/OnlinePlay.hpp"
#include "editor/process/ClientLauncher.hpp"
#include "editor/server/ServerPanelState.hpp"
#include "editor/server/ServerSupervisor.hpp"

namespace rtype::editor {

/// @brief What the editor keeps from one App to the next.
///
/// @details The editor lives in an App, and a rebuild (a reload, or Play online switching between standalone and a
/// client) replaces the App, so the EditorState goes with it. What must survive is here: the child server and the
/// detached clients (a rebuild must not kill them), the Console, its capture and its input line's history, what the
/// Server panel remembers, and the steps of Play online. The program creates one session and gives it to every
/// EditorPlugin it builds (EditorOptions::session); without one, each plugin makes its own.
struct RTYPE_EDITOR_API EditorSession {
    /// @param captureOutput Whether the Console shows the program's output (console::OutputCapture).
    explicit EditorSession(bool captureOutput);
    ~EditorSession() = default;
    EditorSession(const EditorSession& other) = delete;
    EditorSession& operator=(const EditorSession& other) = delete;
    EditorSession(EditorSession&& other) = delete;
    EditorSession& operator=(EditorSession&& other) = delete;

    std::shared_ptr<console::ConsoleLog> console;          ///< The program's output; null if it could not be captured.
    std::unique_ptr<console::OutputCapture> capture;       ///< Fills the console; destroyed before it.
    console::ScriptPrompt prompt;                          ///< The Console's input line: runs Luau, with its history.
    std::unique_ptr<server::ServerSupervisor> supervisor;  ///< The server run as a child process (Server panel).
    std::unique_ptr<process::ClientLauncher> clients;      ///< The detached clients; both stop their processes on exit.
    server::ServerPanelState serverPanel;                  ///< What the Server panel remembers between frames.
    play::OnlinePlay online;                               ///< Play online: standalone, or a client of the server.
    std::string reopenScene;  ///< The scene to open again once the editor is standalone again; consumed by the build.
};

}  // namespace rtype::editor
