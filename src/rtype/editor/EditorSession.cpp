/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorSession
*/

#include "EditorSession.hpp"

#include <exception>
#include <iostream>
#include <memory>

#include "editor/console/ConsoleLog.hpp"
#include "editor/console/OutputCapture.hpp"
#include "editor/process/ClientLauncher.hpp"
#include "editor/server/ServerSupervisor.hpp"

namespace rtype::editor {

EditorSession::EditorSession(const bool captureOutput) {
    if (captureOutput) {
        try {
            console = std::make_shared<console::ConsoleLog>();
            capture = std::make_unique<console::OutputCapture>(console);
        } catch (const std::exception& error) {
            console.reset();
            std::cerr << "[editor] warning: the Console shows nothing: " << error.what() << '\n';
        }
    }
    // The server and the clients write to the Console too; they are stopped when the session goes (see the members).
    supervisor = std::make_unique<server::ServerSupervisor>(console);
    clients = std::make_unique<process::ClientLauncher>(console);
}

}  // namespace rtype::editor
