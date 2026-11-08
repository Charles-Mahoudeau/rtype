/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorConsoleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "editor/console/ConsoleLog.hpp"
#include "editor/console/OutputCapture.hpp"
#include "editor/console/ScriptPrompt.hpp"
#include "editor/shell/TextEditor.hpp"

namespace {
namespace console = rtype::editor::console;

/// @return The texts of @p log's lines.
std::vector<std::string> textsOf(const console::ConsoleLog& log) {
    std::vector<std::string> texts;
    for (const console::Line& line : log.getLines()) {
        texts.push_back(line.text);
    }
    return texts;
}
}  // namespace

TEST(EditorConsole, TellsErrorsAndWarningsInAnyCase) {
    EXPECT_EQ(console::levelOf("Error: no project"), console::Level::kError);
    EXPECT_EQ(console::levelOf("[ScriptSystem] runtime ERROR"), console::Level::kError);
    EXPECT_EQ(console::levelOf("[vulkan] warning: no layer"), console::Level::kWarning);
    EXPECT_EQ(console::levelOf("[server] player 1 wins"), console::Level::kInfo);
}

TEST(EditorConsole, CutsWhatIsWrittenIntoLines) {
    console::ConsoleLog log;
    log.write("first\nsec");
    EXPECT_EQ(textsOf(log), (std::vector<std::string>{"first"}));  // the second waits for its end
    log.write("ond\r\nthird\n");
    EXPECT_EQ(textsOf(log), (std::vector<std::string>{"first", "second", "third"}));
    EXPECT_EQ(log.getAdded(), 3U);
}

TEST(EditorConsole, KeepsTheLastLinesOnly) {
    console::ConsoleLog log;
    for (std::size_t line = 0; line < console::ConsoleLog::kCapacity + 10; ++line) {
        log.addLine(std::format("line {}", line));
    }
    const std::vector<std::string> texts = textsOf(log);
    ASSERT_EQ(texts.size(), console::ConsoleLog::kCapacity);
    EXPECT_EQ(texts.front(), "line 10");
    EXPECT_EQ(log.getAdded(), console::ConsoleLog::kCapacity + 10);
}

TEST(EditorConsole, ForgetsItsLinesWhenCleared) {
    console::ConsoleLog log;
    log.addLine("Error: boom");
    log.clear();
    EXPECT_TRUE(log.getLines().empty());
}

TEST(EditorConsole, CapturesWhatTheProgramWritesWithCAndCpp) {
    const auto log = std::make_shared<console::ConsoleLog>();
    {
        const console::OutputCapture capture{log};
        std::printf("captured by printf\n");  // what Luau's print does
        std::cerr << "captured by cerr\n";
        std::cout << "captured by cout" << std::endl;
    }
    const std::vector<std::string> texts = textsOf(*log);
    EXPECT_NE(std::ranges::find(texts, "captured by printf"), texts.end());
    EXPECT_NE(std::ranges::find(texts, "captured by cerr"), texts.end());
    EXPECT_NE(std::ranges::find(texts, "captured by cout"), texts.end());
}

TEST(EditorTextEditor, RunsTheEditorOnTheFileInTheBackground) {
#ifdef _WIN32
    EXPECT_EQ(rtype::editor::shell::buildOpenCommand("code", "C:/game/a b.actor"),
              "start \"\" code \"C:/game/a b.actor\" >NUL 2>&1");
#else
    EXPECT_EQ(rtype::editor::shell::buildOpenCommand("code", "/game/a b.actor"),
              "code '/game/a b.actor' >/dev/null 2>&1 &");
    EXPECT_EQ(rtype::editor::shell::buildOpenCommand("vim", "/game/it's.actor"),
              "vim '/game/it'\\''s.actor' >/dev/null 2>&1 &");
#endif
}

TEST(EditorTextEditor, DefaultsToAnEditorThatCanRun) {
    const std::string editor = rtype::editor::shell::defaultTextEditor();
    EXPECT_TRUE(editor.contains("ode") || editor == "codium" || editor == "xdg-open") << editor;
}

TEST(EditorTextEditor, RefusesACommandThatDoesNotExist) {
#ifndef _WIN32
    EXPECT_FALSE(rtype::editor::shell::isCommandAvailable("no-such-editor-for-the-tests"));
    EXPECT_FALSE(rtype::editor::shell::openInTextEditor("no-such-editor-for-the-tests", "/game/a.actor"));
    EXPECT_TRUE(rtype::editor::shell::isCommandAvailable("sh -c"));
#endif
}

namespace {
/// @brief A prompt over a fake game: `x` gives 1, `fail` raises, anything else gives its own text.
struct Prompt {
    console::ScriptPrompt prompt;
    std::vector<std::string> evaluated;  ///< The lines the backend ran.
    int completions{0};                  ///< How many times the backend was asked to complete.

    Prompt() {
        prompt.setBackend({
            .evaluate =
                [this](const std::string_view line) {
                    evaluated.emplace_back(line);
                    if (line == "fail") {
                        return console::ScriptOutcome{.ok = false, .text = "console:1: boom"};
                    }
                    return console::ScriptOutcome{.ok = true, .text = line == "x" ? "1" : ""};
                },
            .complete =
                [this](const std::string_view word) {
                    ++completions;
                    std::vector<std::string> words;
                    for (const std::string candidate : {"config", "config.player", "config.speed", "math"}) {
                        if (candidate.starts_with(word) && candidate != word) {
                            words.push_back(candidate);
                        }
                    }
                    return words;
                },
        });
    }
};
}  // namespace

TEST(EditorScriptPrompt, HasNoInputLineWithoutABackend) {
    console::ScriptPrompt prompt;
    console::ConsoleLog log;

    prompt.submit("x", log);

    EXPECT_FALSE(prompt.hasBackend());
    EXPECT_TRUE(log.getLines().empty());
}

TEST(EditorScriptPrompt, WritesTheLineAndWhatItGaveInTheConsole) {
    Prompt fixture;
    console::ConsoleLog log;

    fixture.prompt.submit("  x  ", log);
    fixture.prompt.submit("config.speed = 3", log);

    EXPECT_EQ(textsOf(log), (std::vector<std::string>{"> x", "1", "> config.speed = 3"}));
    EXPECT_EQ(fixture.evaluated, (std::vector<std::string>{"x", "config.speed = 3"}));
}

TEST(EditorScriptPrompt, WritesAnErrorAsAnError) {
    Prompt fixture;
    console::ConsoleLog log;

    fixture.prompt.submit("fail", log);

    const std::vector<console::Line> lines = log.getLines();
    ASSERT_EQ(lines.size(), 2U);
    EXPECT_EQ(lines[1].text, "error: console:1: boom");
    EXPECT_EQ(lines[1].level, console::Level::kError);
}

TEST(EditorScriptPrompt, IgnoresABlankLine) {
    Prompt fixture;
    console::ConsoleLog log;

    fixture.prompt.submit(" \t ", log);

    EXPECT_TRUE(fixture.evaluated.empty());
    EXPECT_TRUE(log.getLines().empty());
}

TEST(EditorScriptPrompt, BrowsesTheHistoryAndComesBackToWhatWasTyped) {
    Prompt fixture;
    console::ConsoleLog log;
    fixture.prompt.submit("one", log);
    fixture.prompt.submit("two", log);
    fixture.prompt.submit("two", log);  // twice in a row: once in the history

    EXPECT_EQ(fixture.prompt.older("dra").value_or(""), "two");
    EXPECT_EQ(fixture.prompt.older("two").value_or(""), "one");
    EXPECT_FALSE(fixture.prompt.older("one").has_value());  // the oldest
    EXPECT_EQ(fixture.prompt.newer().value_or(""), "two");
    EXPECT_EQ(fixture.prompt.newer().value_or(""), "dra");
    EXPECT_FALSE(fixture.prompt.newer().has_value());
}

TEST(EditorScriptPrompt, FindsTheNameAtTheEndOfTheText) {
    EXPECT_EQ(console::ScriptPrompt::wordAt("config.player.si"), "config.player.si");
    EXPECT_EQ(console::ScriptPrompt::wordAt("x = math."), "math.");
    EXPECT_EQ(console::ScriptPrompt::wordAt("f(config.pl"), "config.pl");
    EXPECT_EQ(console::ScriptPrompt::wordAt("x = "), "");
    EXPECT_EQ(console::ScriptPrompt::wordAt(""), "");
}

TEST(EditorScriptPrompt, FindsTheStartTheCompletionsShare) {
    EXPECT_EQ(console::ScriptPrompt::commonPrefix({"config.player", "config.speed"}), "config.");
    EXPECT_EQ(console::ScriptPrompt::commonPrefix({"config"}), "config");
    EXPECT_EQ(console::ScriptPrompt::commonPrefix({"a", "b"}), "");
    EXPECT_EQ(console::ScriptPrompt::commonPrefix({}), "");
}

TEST(EditorScriptPrompt, CompletesTheNameBeforeTheCursorAndAsksOncePerName) {
    Prompt fixture;
    std::size_t hidden = 0;

    const std::vector<std::string> first = fixture.prompt.suggestions("x = conf", hidden);
    const std::vector<std::string> again = fixture.prompt.suggestions("y = conf", hidden);

    EXPECT_EQ(first, (std::vector<std::string>{"config", "config.player", "config.speed"}));
    EXPECT_EQ(again, first);
    EXPECT_EQ(fixture.completions, 1);
    EXPECT_EQ(hidden, 0U);
    EXPECT_TRUE(fixture.prompt.suggestions("x = ", hidden).empty());  // no name: nothing to ask
    EXPECT_EQ(fixture.completions, 1);
}

TEST(EditorScriptPrompt, AsksAgainOnceTheModulesChanged) {
    Prompt fixture;
    std::size_t hidden = 0;
    static_cast<void>(fixture.prompt.suggestions("conf", hidden));

    fixture.prompt.forget();
    static_cast<void>(fixture.prompt.suggestions("conf", hidden));

    EXPECT_EQ(fixture.completions, 2);
}

TEST(EditorScriptPrompt, ShowsAFewCompletionsAndCountsTheOthers) {
    console::ScriptPrompt prompt;
    prompt.setBackend({.evaluate = [](std::string_view) { return console::ScriptOutcome{.ok = true, .text = ""}; },
                       .complete = [](std::string_view) {
                           std::vector<std::string> words;
                           for (int index = 0; index < 20; ++index) {
                               words.push_back(std::format("word{:02}", index));
                           }
                           return words;
                       }});
    std::size_t hidden = 0;

    const std::vector<std::string> shown = prompt.suggestions("w", hidden);

    EXPECT_EQ(shown.size(), console::ScriptPrompt::kMaxSuggestions);
    EXPECT_EQ(hidden, 20U - console::ScriptPrompt::kMaxSuggestions);
}
