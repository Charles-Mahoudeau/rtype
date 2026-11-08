/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPrompt
*/

#include "ScriptPrompt.hpp"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "editor/console/ConsoleLog.hpp"

namespace rtype::editor::console {

namespace {
/// @return Whether @p letter can be part of a dotted name.
bool isNameLetter(const char letter) noexcept {
    return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') || (letter >= '0' && letter <= '9') ||
           letter == '_' || letter == '.' || letter == ':';
}

/// @brief Adds the lines of @p text to @p log, each as it is.
void addLines(ConsoleLog& log, const std::string& text) {
    std::istringstream lines{text};
    std::string line;
    while (std::getline(lines, line)) {
        log.addLine(line);
    }
}
}  // namespace

void ScriptPrompt::setBackend(ScriptBackend backend) {
    _backend = std::move(backend);
    forget();
}

bool ScriptPrompt::hasBackend() const noexcept { return static_cast<bool>(_backend.evaluate); }

void ScriptPrompt::submit(const std::string_view line, ConsoleLog& log) {
    const std::size_t first = line.find_first_not_of(" \t");
    if (first == std::string_view::npos || !hasBackend()) {
        return;
    }
    const std::string_view trimmed = line.substr(first, line.find_last_not_of(" \t") - first + 1);
    if (_history.empty() || _history.back() != trimmed) {
        _history.emplace_back(trimmed);
        if (_history.size() > kHistoryCapacity) {
            _history.erase(_history.begin());
        }
    }
    _cursor = _history.size();
    _draft.clear();
    forget();
    log.addLine("> " + std::string{trimmed});
    const ScriptOutcome outcome = _backend.evaluate(trimmed);
    if (!outcome.ok) {
        addLines(log, "error: " + outcome.text);
    } else if (!outcome.text.empty()) {
        addLines(log, outcome.text);
    }
}

std::string_view ScriptPrompt::wordAt(const std::string_view text) noexcept {
    std::size_t begin = text.size();
    while (begin > 0 && isNameLetter(text.at(begin - 1))) {
        --begin;
    }
    return text.substr(begin);
}

std::string ScriptPrompt::commonPrefix(const std::vector<std::string>& words) {
    if (words.empty()) {
        return {};
    }
    std::string prefix = words.front();
    for (const std::string& word : words) {
        std::size_t length = 0;
        while (length < prefix.size() && length < word.size() && prefix.at(length) == word.at(length)) {
            ++length;
        }
        prefix.resize(length);
    }
    return prefix;
}

std::vector<std::string> ScriptPrompt::suggestions(const std::string_view text, std::size_t& hidden) {
    hidden = 0;
    const std::string_view word = wordAt(text);
    if (word.empty() || !_backend.complete) {
        return {};
    }
    if (!_hasCompletions || _word != word) {
        _word = word;
        _completions = _backend.complete(word);
        _hasCompletions = true;
    }
    if (_completions.size() > kMaxSuggestions) {
        hidden = _completions.size() - kMaxSuggestions;
        return {_completions.begin(), _completions.begin() + static_cast<std::ptrdiff_t>(kMaxSuggestions)};
    }
    return _completions;
}

void ScriptPrompt::forget() noexcept {
    _hasCompletions = false;
    _word.clear();
    _completions.clear();
}

std::optional<std::string> ScriptPrompt::older(const std::string_view current) {
    if (_history.empty() || _cursor == 0) {
        return std::nullopt;
    }
    if (_cursor == _history.size()) {
        _draft = current;
    }
    --_cursor;
    return _history.at(_cursor);
}

std::optional<std::string> ScriptPrompt::newer() {
    if (_cursor >= _history.size()) {
        return std::nullopt;
    }
    ++_cursor;
    return _cursor == _history.size() ? _draft : _history.at(_cursor);
}

}  // namespace rtype::editor::console
