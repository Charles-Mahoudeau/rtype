/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextEditor
*/

#include "TextEditor.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <system_error>

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

bool isCommandAvailable(std::string_view editor) {
#ifdef _WIN32
    return !editor.empty();  // `start` finds its own program, or fails visibly
#else
    if (editor.starts_with("open -a ")) {
        return true;  // macOS `open` finds the application by its name
    }
    const std::string_view program = editor.substr(0, editor.find(' '));
    if (program.empty()) {
        return false;
    }
    std::error_code error;
    if (program.contains('/')) {
        return std::filesystem::exists(std::filesystem::path{program}, error);
    }
    // NOLINTNEXTLINE(concurrency-mt-unsafe): read on the main thread
    const char* path = std::getenv("PATH");
    std::string_view rest = path != nullptr ? path : "";
    while (!rest.empty()) {
        const std::size_t cut = std::min(rest.find(':'), rest.size());
        if (std::filesystem::exists(std::filesystem::path{rest.substr(0, cut)} / program, error)) {
            return true;
        }
        rest.remove_prefix(std::min(cut + 1, rest.size()));
    }
    return false;
#endif
}

std::string defaultTextEditor() {
#ifdef __APPLE__
    return "open -a \"Visual Studio Code\"";
#else
    // Visual Studio Code, else what is installed in its place; `code` stays the answer when none is.
    for (const char* candidate : {"code", "codium", "xdg-open"}) {
        if (isCommandAvailable(candidate)) {
            return candidate;
        }
    }
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
    if (!isCommandAvailable(editor)) {
        return false;  // run in the background, a missing command would otherwise fail out of sight
    }
    // The user's own command, on the main thread.
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c,bugprone-command-processor)
    return std::system(buildOpenCommand(editor, file).c_str()) == 0;
}

}  // namespace rtype::editor::shell
