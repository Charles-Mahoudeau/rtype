/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "editor/Export.hpp"

/// How the editor starts `aylo-runtime`, the generic executable that runs a game: its place and its command lines.
namespace rtype::editor::process {

/// @return Where `aylo-runtime` is: next to the running editor, with the extension of the platform. The file may not
/// exist (the path is returned anyway, so a message can name it).
[[nodiscard]] RTYPE_EDITOR_API std::filesystem::path runtimePath();

/// @brief The command line of a server: `<game> --server --port <port> [--admin-port <p> --admin-token <t>] <extra>`.
/// @param game The game folder, the first argument.
/// @param port The port the server listens on.
/// @param adminPort The local port of the admin channel; 0 leaves the channel off.
/// @param adminToken The secret of the admin channel.
/// @param extra More flags, given as they are (they reach the game's `engine.args`).
[[nodiscard]] RTYPE_EDITOR_API std::vector<std::string> serverArguments(const std::filesystem::path& game,
                                                                        std::uint16_t port, std::uint16_t adminPort,
                                                                        const std::string& adminToken,
                                                                        const std::vector<std::string>& extra);

/// @brief The command line of a client: `<game> --host <host> --port <port> <extra>`.
[[nodiscard]] RTYPE_EDITOR_API std::vector<std::string> clientArguments(const std::filesystem::path& game,
                                                                        const std::string& host, std::uint16_t port,
                                                                        const std::vector<std::string>& extra);

/// @brief Cuts @p text into arguments at the spaces; double quotes keep a space in one argument.
[[nodiscard]] RTYPE_EDITOR_API std::vector<std::string> splitArguments(const std::string& text);

}  // namespace rtype::editor::process
