/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsoleLog
*/

#include "ConsoleLog.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rtype::editor::console {

namespace {
/// @return Whether @p text holds @p word, in any case.
bool contains(std::string_view text, std::string_view word) {
    const auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
    return !std::ranges::search(text, word, {}, lower, lower).empty();
}
}  // namespace

Level levelOf(std::string_view text) {
    if (contains(text, "error")) {
        return Level::kError;
    }
    return contains(text, "warning") ? Level::kWarning : Level::kInfo;
}

void ConsoleLog::write(std::string_view bytes) {
    const std::scoped_lock lock{_mutex};
    for (const char c : bytes) {
        if (c == '\n') {
            push(std::exchange(_pending, {}));
        } else if (c != '\r') {
            _pending += c;
        }
    }
}

void ConsoleLog::addLine(std::string_view text) {
    const std::scoped_lock lock{_mutex};
    push(std::string{text});
}

std::vector<Line> ConsoleLog::getLines() const {
    const std::scoped_lock lock{_mutex};
    return {_lines.begin(), _lines.end()};
}

std::size_t ConsoleLog::getAdded() const {
    const std::scoped_lock lock{_mutex};
    return _added;
}

void ConsoleLog::clear() {
    const std::scoped_lock lock{_mutex};
    _lines.clear();
}

void ConsoleLog::push(std::string text) {
    const Level level = levelOf(text);
    _lines.push_back(Line{.text = std::move(text), .level = level});
    if (_lines.size() > kCapacity) {
        _lines.pop_front();
    }
    ++_added;
}

}  // namespace rtype::editor::console
