/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorServerTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "editor/console/ConsoleLog.hpp"
#include "editor/process/ClientLauncher.hpp"
#include "editor/process/Runtime.hpp"
#include "editor/server/AdminSession.hpp"
#include "editor/server/ServerSupervisor.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminServer.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"
#include "rtype/network/server/NetServer.hpp"

namespace {
namespace admin = rtype::network::admin;
namespace console = rtype::editor::console;
namespace process = rtype::editor::process;
namespace server = rtype::editor::server;
namespace network = rtype::network;
using std::chrono::milliseconds;

constexpr const char* kToken = "tok3n";

TEST(EditorRuntime, BuildsTheServerCommandLine) {
    const auto args = process::serverArguments("game", 4242, 5000, "secret", {"--name", "boss"});
    const std::vector<std::string> expected{"game", "--server",      "--port", "4242",   "--admin-port",
                                            "5000", "--admin-token", "secret", "--name", "boss"};
    EXPECT_EQ(args, expected);
}

TEST(EditorRuntime, LeavesTheAdminChannelOffWithoutAPort) {
    const auto args = process::serverArguments("game", 4242, 0, "secret", {});
    const std::vector<std::string> expected{"game", "--server", "--port", "4242"};
    EXPECT_EQ(args, expected);
}

TEST(EditorRuntime, BuildsTheClientCommandLine) {
    const auto args = process::clientArguments("game", "127.0.0.1", 4242, {"--name", "p2"});
    const std::vector<std::string> expected{"game", "--host", "127.0.0.1", "--port", "4242", "--name", "p2"};
    EXPECT_EQ(args, expected);
}

TEST(EditorRuntime, SplitsTypedArgumentsAtSpacesAndKeepsQuotes) {
    const std::vector<std::string> expected{"--name", "two words", "--x"};
    EXPECT_EQ(process::splitArguments("  --name   \"two words\" --x "), expected);
    EXPECT_TRUE(process::splitArguments("   ").empty());
    EXPECT_EQ(process::splitArguments("\"\"").size(), 1U);  // an empty quoted argument is kept
}

/// A server, its admin channel, and the session under test. The clock is the test's: only `now` moves it.
struct SessionFixture {
    network::NetworkSettings settings = [] {
        network::NetworkSettings value;
        value.port = 0;
        return value;
    }();
    network::server::NetServer netServer{settings};
    admin::AdminServer adminServer{0, kToken, network::Clock::now()};
    server::AdminSession session{adminServer.localPort(), kToken};
    server::AdminSession::Clock::time_point now = server::AdminSession::Clock::now();

    SessionFixture() {
        adminServer.setExtensionHandler([this](const admin::ExtensionRequest& request) -> std::optional<admin::Reply> {
            const auto world = admin::decodeWorldRequest(request);
            if (!world) {
                return std::nullopt;
            }
            ++worldRequests;
            if (std::holds_alternative<admin::EntitiesRequest>(*world)) {
                admin::EntitiesReply reply{.total = 2, .first = 0, .entries = {{.entity = 7, .name = "Ship"}}};
                return admin::Reply{admin::encodeWorldReply(reply)};
            }
            if (std::holds_alternative<admin::SpawnRequest>(*world)) {
                ++spawns;
                return admin::Reply{admin::encodeWorldReply(admin::SpawnReply{.entity = 99})};
            }
            if (const auto* run = std::get_if<admin::RunRequest>(&*world)) {
                paused = run->action == admin::RunAction::kPause ? true : paused;
                return admin::Reply{admin::encodeWorldReply(admin::RunReply{.paused = paused})};
            }
            return std::nullopt;
        });
    }

    /// Lets the server answer what arrived, then the session read it.
    void serve() {
        netServer.poll(network::Clock::now());
        adminServer.poll(network::Clock::now(), netServer, admin::GameStatus{.tick = 12, .entityCount = 3});
    }

    /// Runs the session and the server until @p done, at most two seconds, the session's clock moving 10 ms a turn.
    bool until(const std::function<bool()>& done) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (std::chrono::steady_clock::now() < deadline) {
            now += milliseconds{10};
            session.update(now);
            serve();
            std::this_thread::sleep_for(milliseconds{1});
            session.update(now);
            if (done()) {
                return true;
            }
        }
        return false;
    }

    int worldRequests = 0;
    int spawns = 0;
    bool paused = false;
};

TEST(EditorAdminSession, PollsTheStatusAndTheClients) {
    SessionFixture fixture;
    EXPECT_FALSE(fixture.session.isConnected());
    ASSERT_TRUE(fixture.until([&] { return fixture.session.status() && fixture.session.clients(); }));
    EXPECT_TRUE(fixture.session.isConnected());
    EXPECT_EQ(fixture.session.status()->tick, 12U);
    EXPECT_EQ(fixture.session.status()->entityCount, 3U);
    EXPECT_EQ(fixture.session.clients()->total, 0U);
}

TEST(EditorAdminSession, SendsALostRequestAgainThenGivesUp) {
    SessionFixture fixture;
    fixture.session.update(fixture.now);  // sends the polls; the server is not served
    const std::size_t first = fixture.session.pendingCount();
    ASSERT_GT(first, 0U);
    for (int attempt = 1; attempt < server::AdminSession::kMaxAttempts + 1; ++attempt) {
        fixture.now += server::AdminSession::kRetryDelay;
        fixture.session.update(fixture.now);
    }
    // Every poll was sent kMaxAttempts times, gave up, and the next poll (not due yet) has not replaced it.
    EXPECT_FALSE(fixture.session.isConnected());
    EXPECT_LE(fixture.session.pendingCount(), first);
    EXPECT_EQ(fixture.session.lastError(), "");  // never connected: a silent server is not an error
}

TEST(EditorAdminSession, AnswersToARequestSentTwiceFillThePendingOnce) {
    SessionFixture fixture;
    fixture.session.update(fixture.now);
    fixture.now += server::AdminSession::kRetryDelay;
    fixture.session.update(fixture.now);  // sent again: the server now holds both copies
    std::this_thread::sleep_for(milliseconds{5});
    fixture.serve();
    std::this_thread::sleep_for(milliseconds{5});
    fixture.session.update(fixture.now);  // the reply to the first copy is ignored, the second one counts
    EXPECT_TRUE(fixture.session.isConnected());
    ASSERT_TRUE(fixture.session.status().has_value());
}

TEST(EditorAdminSession, ReportsACommandTheServerDoesNotAnswer) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isConnected(); }));
    fixture.session.kick(1);
    for (int turn = 0; turn <= server::AdminSession::kMaxAttempts; ++turn) {
        fixture.now += server::AdminSession::kRetryDelay;
        fixture.session.update(fixture.now);
    }
    EXPECT_EQ(fixture.session.lastError(), "the server did not answer");
}

TEST(EditorAdminSession, KickingAnUnknownClientReportsIt) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isConnected(); }));
    fixture.session.kick(42);
    ASSERT_TRUE(fixture.until([&] { return !fixture.session.lastError().empty(); }));
    EXPECT_EQ(fixture.session.lastError(), "no such client");
}

TEST(EditorAdminSession, SetsTheSimulationAndShowsWhatTheServerKept) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.simulation().has_value(); }));
    EXPECT_FALSE(fixture.session.simulation()->isActive());
    fixture.session.setSimulation({.latencyMs = 100, .jitterMs = 9999, .lossPercent = 5.0F});
    ASSERT_TRUE(
        fixture.until([&] { return fixture.session.simulation() && fixture.session.simulation()->isActive(); }));
    EXPECT_EQ(fixture.session.simulation()->latencyMs, 100);
    EXPECT_EQ(fixture.session.simulation()->jitterMs,
              rtype::network::simulation::kMaxDelayMs);  // clamped by the server
    EXPECT_FLOAT_EQ(fixture.session.simulation()->lossPercent, 5.0F);
}

TEST(EditorAdminSession, WatchesTheEntityListOnlyWhileAskedTo) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isConnected(); }));
    fixture.session.watchEntities(0);
    ASSERT_TRUE(fixture.until([&] { return fixture.session.entities().has_value(); }));
    EXPECT_EQ(fixture.session.entities()->total, 2U);
    ASSERT_EQ(fixture.session.entities()->entries.size(), 1U);
    EXPECT_EQ(fixture.session.entities()->entries.front().name, "Ship");

    // The watch lapses after kWatchFor: let what is in flight finish, then no more entity requests are made.
    fixture.now += server::AdminSession::kWatchFor + milliseconds{50};
    ASSERT_TRUE(fixture.until([&] { return fixture.session.pendingCount() == 0; }));
    const int before = fixture.worldRequests;
    for (int turn = 0; turn < 20; ++turn) {
        fixture.now += server::AdminSession::kPollInterval;
        fixture.session.update(fixture.now);
        std::this_thread::sleep_for(milliseconds{2});
        fixture.serve();
    }
    EXPECT_EQ(fixture.worldRequests, before);
}

TEST(EditorAdminSession, SpawnIsNeverSentTwice) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isConnected(); }));
    fixture.session.spawn("Boss");
    // Not served for a while: a lost spawn would be sent again if it were retryable.
    for (int turn = 0; turn < 4; ++turn) {
        fixture.now += server::AdminSession::kRetryDelay;
        fixture.session.update(fixture.now);
    }
    for (int turn = 0; turn < 20; ++turn) {
        std::this_thread::sleep_for(milliseconds{2});
        fixture.serve();
    }
    EXPECT_EQ(fixture.spawns, 1);
}

TEST(EditorAdminSession, SpawnTellsTheNewEntityAndPauseTheState) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isPaused().has_value(); }));
    EXPECT_FALSE(*fixture.session.isPaused());
    fixture.session.spawn("Boss");
    fixture.session.run(admin::RunAction::kPause);
    ASSERT_TRUE(
        fixture.until([&] { return fixture.session.lastSpawned().has_value() && *fixture.session.isPaused(); }));
    EXPECT_EQ(*fixture.session.lastSpawned(), 99U);
}

TEST(EditorAdminSession, SaysWhenTheServerGoesSilent) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isConnected(); }));
    EXPECT_FALSE(fixture.session.isLost());
    fixture.now += server::AdminSession::kLostAfter + milliseconds{100};
    fixture.session.update(fixture.now);
    EXPECT_TRUE(fixture.session.isLost());
    // It comes back: a reply ends the loss.
    ASSERT_TRUE(fixture.until([&] { return !fixture.session.isLost(); }));
}

TEST(EditorAdminSession, ReportsAnEntityRequestTheServerDoesNotSupport) {
    SessionFixture fixture;
    ASSERT_TRUE(fixture.until([&] { return fixture.session.isConnected(); }));
    fixture.session.watchEntity(123, 0);  // the fixture answers inspect with "unsupported"
    ASSERT_TRUE(fixture.until([&] { return !fixture.session.lastError().empty(); }));
    EXPECT_FALSE(fixture.session.inspected().has_value());
}

#ifndef _WIN32

/// A folder in the temp directory, with a fake `aylo-runtime` (a shell script) in it.
struct FakeRuntime {
    std::filesystem::path directory;
    std::filesystem::path runtime;

    explicit FakeRuntime(const std::string& body) {
        static int counter = 0;
        directory = std::filesystem::temp_directory_path() /
                    ("aylo-editor-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                     "-" + std::to_string(counter++));
        std::filesystem::create_directories(directory);
        runtime = directory / "aylo-runtime";
        {
            std::ofstream file{runtime};
            file << "#!/bin/sh\n" << body << "\n";
        }
        std::filesystem::permissions(runtime, std::filesystem::perms::owner_all);
    }
    FakeRuntime(const FakeRuntime&) = delete;
    FakeRuntime& operator=(const FakeRuntime&) = delete;
    ~FakeRuntime() {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
};

/// Updates @p update until @p done, at most five seconds.
bool waitFor(const std::function<void()>& update, const std::function<bool()>& done) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (std::chrono::steady_clock::now() < deadline) {
        update();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(milliseconds{5});
    }
    return false;
}

bool contains(const console::ConsoleLog& log, const std::string& text) {
    for (const console::Line& line : log.getLines()) {
        if (line.text.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

server::ServerOptions optionsOf(const FakeRuntime& fake) {
    return server::ServerOptions{
        .gameDirectory = fake.directory, .port = 4300, .extraArgs = {"--name", "x"}, .runtime = fake.runtime};
}

TEST(EditorServerSupervisor, StartsTheRuntimeWithTheServerFlagsAndShowsItsOutput) {
    FakeRuntime fake{"echo \"args: $@\"\nsleep 30"};
    auto sink = std::make_shared<console::ConsoleLog>();
    server::ServerSupervisor supervisor{sink};
    ASSERT_TRUE(supervisor.start(optionsOf(fake)).has_value());
    EXPECT_EQ(supervisor.status(), server::ServerStatus::kStarting);
    EXPECT_NE(supervisor.adminPort(), 0);
    ASSERT_TRUE(waitFor([&] { supervisor.update(); }, [&] { return contains(supervisor.log(), "args:"); }));
    EXPECT_TRUE(contains(supervisor.log(), "--server --port 4300 --admin-port"));
    EXPECT_TRUE(contains(supervisor.log(), "--name x"));
    EXPECT_TRUE(contains(*sink, "[server] args:"));
    supervisor.stop();
    EXPECT_EQ(supervisor.status(), server::ServerStatus::kStopped);
    EXPECT_EQ(supervisor.session(), nullptr);
}

TEST(EditorServerSupervisor, RefusesWhatCannotStart) {
    FakeRuntime fake{"sleep 30"};
    server::ServerSupervisor supervisor;
    server::ServerOptions options = optionsOf(fake);

    options.gameDirectory = fake.directory / "missing";
    EXPECT_FALSE(supervisor.start(options).has_value());
    options = optionsOf(fake);
    options.port = 0;
    EXPECT_FALSE(supervisor.start(options).has_value());
    options = optionsOf(fake);
    options.runtime = fake.directory / "no-runtime";
    const auto missing = supervisor.start(options);
    ASSERT_FALSE(missing.has_value());
    EXPECT_NE(missing.error().find("no-runtime"), std::string::npos);
    EXPECT_EQ(supervisor.status(), server::ServerStatus::kStopped);

    ASSERT_TRUE(supervisor.start(optionsOf(fake)).has_value());
    EXPECT_FALSE(supervisor.start(optionsOf(fake)).has_value());  // already running
}

TEST(EditorServerSupervisor, NoticesAServerThatEndsByItself) {
    FakeRuntime fake{"echo bye\nexit 3"};
    server::ServerSupervisor supervisor;
    ASSERT_TRUE(supervisor.start(optionsOf(fake)).has_value());
    ASSERT_TRUE(
        waitFor([&] { supervisor.update(); }, [&] { return supervisor.status() == server::ServerStatus::kExited; }));
    EXPECT_EQ(supervisor.exitCode(), 3);
    EXPECT_TRUE(contains(supervisor.log(), "bye"));
    EXPECT_TRUE(contains(supervisor.log(), "ended with code 3"));
    EXPECT_EQ(supervisor.session(), nullptr);
    // After an exit the server can start again.
    EXPECT_TRUE(supervisor.start(optionsOf(fake)).has_value());
}

TEST(EditorServerSupervisor, RestartsWithTheSameOptionsAndAFreshAdminChannel) {
    FakeRuntime fake{"sleep 30"};
    server::ServerSupervisor supervisor;
    ASSERT_TRUE(supervisor.start(optionsOf(fake)).has_value());
    const std::uint16_t before = supervisor.adminPort();
    ASSERT_NE(before, 0);
    ASSERT_TRUE(supervisor.restart().has_value());
    EXPECT_EQ(supervisor.status(), server::ServerStatus::kStarting);
    EXPECT_EQ(supervisor.options().port, 4300);
    EXPECT_NE(supervisor.session(), nullptr);
}

TEST(EditorServerSupervisor, BecomesRunningOnceTheAdminChannelAnswers) {
    // The fake server prints the admin port and token it was given; the test opens the admin channel there.
    FakeRuntime fake{"echo \"$@\"\nsleep 30"};
    server::ServerSupervisor supervisor;
    ASSERT_TRUE(supervisor.start(optionsOf(fake)).has_value());
    ASSERT_TRUE(waitFor([&] { supervisor.update(); }, [&] { return contains(supervisor.log(), "--admin-token"); }));

    std::string port;
    std::string token;
    for (const console::Line& line : supervisor.log().getLines()) {
        if (const auto at = line.text.find("--admin-port "); at != std::string::npos) {
            const auto words = process::splitArguments(line.text.substr(at));
            port = words.at(1);
            token = words.at(3);
        }
    }
    ASSERT_FALSE(port.empty());
    network::NetworkSettings settings;
    settings.port = 0;
    network::server::NetServer netServer{settings};
    admin::AdminServer adminServer{static_cast<std::uint16_t>(std::stoi(port)), token, network::Clock::now()};

    ASSERT_TRUE(waitFor(
        [&] {
            netServer.poll(network::Clock::now());
            adminServer.poll(network::Clock::now(), netServer, admin::GameStatus{});
            supervisor.update();
        },
        [&] { return supervisor.status() == server::ServerStatus::kRunning; }));
    ASSERT_NE(supervisor.session(), nullptr);
    EXPECT_TRUE(supervisor.session()->isConnected());
}

TEST(EditorServerSupervisor, EndsTheServerWhenDestroyed) {
    // `exec` keeps the process id: the script's own is the server's.
    FakeRuntime fake{"echo $$ > \"$(dirname \"$0\")/pid\"\nexec sleep 30"};
    const std::filesystem::path pidFile = fake.directory / "pid";
    long pid = 0;
    {
        server::ServerSupervisor supervisor;
        ASSERT_TRUE(supervisor.start(optionsOf(fake)).has_value());
        ASSERT_TRUE(waitFor(
            [] {}, [&] { return std::filesystem::exists(pidFile) && std::filesystem::file_size(pidFile) > 0; }));
        std::ifstream{pidFile} >> pid;
        ASSERT_GT(pid, 0);
        EXPECT_EQ(::kill(static_cast<pid_t>(pid), 0), 0);  // alive
    }
    EXPECT_NE(::kill(static_cast<pid_t>(pid), 0), 0);  // gone and reaped
}

TEST(EditorServerSupervisor, MakesDifferentSecretsOfTheRightSize) {
    const std::string first = server::ServerSupervisor::makeToken();
    const std::string second = server::ServerSupervisor::makeToken();
    EXPECT_EQ(first.size(), 32U);
    EXPECT_NE(first, second);
    EXPECT_NE(server::ServerSupervisor::findFreePort(), 0);
}

process::ClientOptions clientOptionsOf(const FakeRuntime& fake) {
    return process::ClientOptions{
        .gameDirectory = fake.directory, .host = "127.0.0.1", .port = 4300, .runtime = fake.runtime};
}

TEST(EditorClientLauncher, LaunchesNumberedClientsWithTheirOwnFlags) {
    FakeRuntime fake{"echo \"client $@\"\nsleep 30"};
    auto sink = std::make_shared<console::ConsoleLog>();
    process::ClientLauncher launcher{sink};
    const auto first = launcher.launch(clientOptionsOf(fake), {});
    const auto second = launcher.launch(clientOptionsOf(fake), {"--name", "p2"});
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, 1);
    EXPECT_EQ(*second, 2);
    EXPECT_EQ(launcher.runningCount(), 2U);
    const auto list = launcher.list();
    ASSERT_EQ(list.size(), 2U);
    EXPECT_TRUE(list.at(0).running);
    EXPECT_NE(list.at(0).pid, list.at(1).pid);
    EXPECT_EQ(list.at(1).args, (std::vector<std::string>{"--name", "p2"}));

    ASSERT_TRUE(waitFor([&] { launcher.update(); }, [&] { return contains(*sink, "[client 2] client"); }));
    EXPECT_TRUE(contains(*sink, "--host 127.0.0.1 --port 4300 --name p2"));
}

TEST(EditorClientLauncher, KillsOneClientOrAll) {
    FakeRuntime fake{"sleep 30"};
    process::ClientLauncher launcher;
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(launcher.launch(clientOptionsOf(fake), {}).has_value());
    }
    launcher.kill(2);
    EXPECT_EQ(launcher.runningCount(), 2U);
    EXPECT_FALSE(launcher.list().at(1).running);
    launcher.kill(99);  // unknown: nothing happens
    EXPECT_EQ(launcher.runningCount(), 2U);

    launcher.killAll();
    EXPECT_EQ(launcher.runningCount(), 0U);
    launcher.removeEnded();
    EXPECT_TRUE(launcher.list().empty());
    // Numbers are never reused.
    const auto next = launcher.launch(clientOptionsOf(fake), {});
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(*next, 4);
}

TEST(EditorClientLauncher, NoticesAClientThatEnds) {
    FakeRuntime fake{"exit 5"};
    process::ClientLauncher launcher;
    ASSERT_TRUE(launcher.launch(clientOptionsOf(fake), {}).has_value());
    ASSERT_TRUE(waitFor([&] { launcher.update(); }, [&] { return launcher.runningCount() == 0; }));
    EXPECT_EQ(launcher.list().front().exitCode, 5);
}

TEST(EditorClientLauncher, RestartsEveryClientUnderItsNumberWithItsFlags) {
    FakeRuntime fake{"sleep 30"};
    process::ClientLauncher launcher;
    ASSERT_TRUE(launcher.launch(clientOptionsOf(fake), {"--name", "a"}).has_value());
    ASSERT_TRUE(launcher.launch(clientOptionsOf(fake), {"--name", "b"}).has_value());
    const auto before = launcher.list();
    launcher.kill(1);
    ASSERT_TRUE(launcher.restartAll(clientOptionsOf(fake)).has_value());
    const auto after = launcher.list();
    ASSERT_EQ(after.size(), 2U);
    EXPECT_EQ(launcher.runningCount(), 2U);
    EXPECT_EQ(after.at(0).number, 1);
    EXPECT_EQ(after.at(1).args, (std::vector<std::string>{"--name", "b"}));
    EXPECT_NE(after.at(1).pid, before.at(1).pid);
}

TEST(EditorClientLauncher, RefusesWhatCannotStart) {
    FakeRuntime fake{"sleep 30"};
    process::ClientLauncher launcher;
    process::ClientOptions options = clientOptionsOf(fake);
    options.gameDirectory = fake.directory / "missing";
    EXPECT_FALSE(launcher.launch(options, {}).has_value());
    options = clientOptionsOf(fake);
    options.runtime = fake.directory / "no-runtime";
    EXPECT_FALSE(launcher.launch(options, {}).has_value());
    EXPECT_TRUE(launcher.list().empty());
}

#endif  // _WIN32
}  // namespace
