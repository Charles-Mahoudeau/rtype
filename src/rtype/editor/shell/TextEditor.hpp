/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextEditor
*/

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "editor/Export.hpp"

namespace rtype::editor::shell {

/// @return The command that opens a file in a text editor by default: Visual Studio Code, through `open` on macOS (it
/// needs no `code` command installed) and its `code` command elsewhere.
[[nodiscard]] RTYPE_EDITOR_API std::string defaultTextEditor();

/// @return The shell command that runs @p editor (a command, its arguments included) on @p file, in the background,
/// its output thrown away: the editor does not wait for it, and it holds none of the editor's output.
[[nodiscard]] RTYPE_EDITOR_API std::string buildOpenCommand(std::string_view editor, const std::filesystem::path& file);

/// @brief Opens @p file in @p editor (buildOpenCommand()).
/// @return Whether the shell could run the command; the editor itself may still fail, out of sight.
RTYPE_EDITOR_API bool openInTextEditor(std::string_view editor, const std::filesystem::path& file);

}  // namespace rtype::editor::shell
