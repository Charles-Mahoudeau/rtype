/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ExecutablePath
*/

#pragma once

#include <filesystem>

namespace rtype::engine::system {

/// @return The absolute path of the running executable, symbolic links resolved.
/// @details Unlike the working directory, it does not depend on where the program is launched from: files shipped
/// next to the executable are found the same way through `xmake run`, a terminal or a file manager.
/// @throws std::runtime_error If the operating system cannot tell.
[[nodiscard]] std::filesystem::path getExecutablePath();

/// @return The directory of getExecutablePath().
/// @throws std::runtime_error If the operating system cannot tell.
[[nodiscard]] std::filesystem::path getExecutableDirectory();

}  // namespace rtype::engine::system
