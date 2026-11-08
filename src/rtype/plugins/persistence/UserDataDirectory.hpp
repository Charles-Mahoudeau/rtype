/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UserDataDirectory
*/

#pragma once

#include <filesystem>

#include "plugins/persistence/Export.hpp"

namespace rtype::plugins::persistence {

/// @return The current user's data directory, where a game keeps what must survive (saves): `%APPDATA%` on Windows,
/// `~/Library/Application Support` on macOS, `$XDG_DATA_HOME` or `~/.local/share` elsewhere. Empty when the
/// environment does not tell.
[[nodiscard]] RTYPE_PLUGIN_PERSISTENCE_API std::filesystem::path getUserDataDirectory();

}  // namespace rtype::plugins::persistence
