/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorChildProcessTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <string>
#include <utility>

#include "editor/process/ChildProcess.hpp"

#ifndef _WIN32

namespace {
namespace process = rtype::editor::process;
using std::chrono::milliseconds;

/// @return A spec running @p script in `sh`.
process::ProcessSpec shell(const std::string& script) { return process::ProcessSpec{"sh", {"-c", script}, {}}; }

/// Reads the output of @p child until it ended, so nothing is lost to the race between the exit and the pipe.
std::string readAll(process::ChildProcess& child) {
    std::string out;
    (void)child.waitFor(milliseconds{5000});
    out += child.drainOutput();
    return out;
}
}  // namespace

TEST(EditorChildProcess, RunsAProgramAndKeepsItsExitCode) {
    auto child = process::ChildProcess::spawn(shell("exit 7"));
    ASSERT_TRUE(child.has_value());
    EXPECT_TRUE(child->waitFor(milliseconds{5000}));
    EXPECT_FALSE(child->isRunning());
    ASSERT_TRUE(child->exitCode().has_value());
    EXPECT_EQ(*child->exitCode(), 7);
}

TEST(EditorChildProcess, CapturesOutputAndErrorWithoutBlocking) {
    auto child = process::ChildProcess::spawn(shell("echo out; echo err 1>&2"));
    ASSERT_TRUE(child.has_value());
    const std::string text = readAll(*child);
    EXPECT_NE(text.find("out"), std::string::npos);
    EXPECT_NE(text.find("err"), std::string::npos);
    EXPECT_TRUE(child->drainOutput().empty());
}

TEST(EditorChildProcess, ReportsAMissingProgram) {
    auto child = process::ChildProcess::spawn(process::ProcessSpec{"/no/such/program", {}, {}});
    ASSERT_FALSE(child.has_value());
    EXPECT_NE(child.error().find("/no/such/program"), std::string::npos);
}

TEST(EditorChildProcess, KillEndsARunningProcess) {
    auto child = process::ChildProcess::spawn(shell("sleep 30"));
    ASSERT_TRUE(child.has_value());
    EXPECT_TRUE(child->isRunning());
    EXPECT_NE(child->pid(), 0U);
    child->kill();
    EXPECT_FALSE(child->isRunning());
    ASSERT_TRUE(child->exitCode().has_value());
    EXPECT_LT(*child->exitCode(), 0);  // ended by a signal
}

TEST(EditorChildProcess, StopAsksFirstThenKills) {
    // Ignores SIGTERM: only the kill after the grace delay ends it.
    auto child = process::ChildProcess::spawn(shell("trap '' TERM; while true; do sleep 1; done"));
    ASSERT_TRUE(child.has_value());
    child->stop(milliseconds{100});
    EXPECT_FALSE(child->isRunning());
}

TEST(EditorChildProcess, DestructorEndsTheProcess) {
    std::uint64_t id = 0;
    {
        auto child = process::ChildProcess::spawn(shell("sleep 30"));
        ASSERT_TRUE(child.has_value());
        id = child->pid();
    }
    ASSERT_NE(id, 0U);
    // The process is gone and reaped: signal 0 finds nothing.
    EXPECT_NE(::kill(static_cast<pid_t>(id), 0), 0);
}

TEST(EditorChildProcess, MovingKeepsTheProcessOwned) {
    auto spawned = process::ChildProcess::spawn(shell("sleep 30"));
    ASSERT_TRUE(spawned.has_value());
    process::ChildProcess moved = std::move(*spawned);
    EXPECT_TRUE(moved.isRunning());
    EXPECT_FALSE(spawned->isRunning());  // the moved-from one owns nothing
}

#endif  // _WIN32
