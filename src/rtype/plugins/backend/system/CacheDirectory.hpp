/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CacheDirectory
*/

#pragma once

#include <filesystem>

namespace rtype::plugins::backend::system {

/// @return The current user's cache directory, where a program keeps files it can rebuild (a pipeline cache...):
/// `%LOCALAPPDATA%` on Windows, `~/Library/Caches` on macOS, `$XDG_CACHE_HOME` or `~/.cache` elsewhere. Always
/// writable by the user, even when the game is installed in a read-only directory. Programs make their own folder
/// in it. Empty when the environment does not tell (no home directory).
[[nodiscard]] std::filesystem::path getCacheDirectory();

}  // namespace rtype::plugins::backend::system
