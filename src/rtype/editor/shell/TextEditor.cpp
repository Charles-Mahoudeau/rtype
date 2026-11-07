/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextEditor
*/

#include "TextEditor.hpp"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

namespace rtype::editor::shell {

namespace {
/// @return @p text quoted for the system's shell.
std::string quote(std::string_view text) {
#ifdef _WIN32
    return std::format("\"{}\"", text);  // a Windows path holds no double quote
#else
    std::string quoted = "'";
    for (const char c : text) {
        quoted += c == '\'' ? std::string{"'\\''"} : std::string(1, c);
    }
    return quoted + "'";
#endif
}
}  // namespace

std::string defaultTextEditor() {
#ifdef __APPLE__
    return "open -a \"Visual Studio Code\"";
#else
    return "code";
#endif
}

std::string buildOpenCommand(std::string_view editor, const std::filesystem::path& file) {
#ifdef _WIN32
    return std::format("start \"\" {} {} >NUL 2>&1", editor, quote(file.string()));
#else
    return std::format("{} {} >/dev/null 2>&1 &", editor, quote(file.string()));
#endif
}

bool openInTextEditor(std::string_view editor, const std::filesystem::path& file) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c): the user's own command, on the main thread.
    return std::system(buildOpenCommand(editor, file).c_str()) == 0;
}

}  // namespace rtype::editor::shell
