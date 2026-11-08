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
