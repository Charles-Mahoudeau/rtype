/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorOnlinePlayTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "editor/EditorPlugin.hpp"
#include "editor/EditorSession.hpp"
#include "editor/play/OnlinePlay.hpp"
#include "editor/process/Runtime.hpp"
#include "editor/server/ServerSupervisor.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminServer.hpp"
#include "rtype/network/server/NetServer.hpp"

#ifndef _WIN32

namespace {
namespace admin = rtype::network::admin;
namespace editor = rtype::editor;
namespace play = rtype::editor::play;
namespace server = rtype::editor::server;
namespace network = rtype::network;
using rtype::ecs::RunMode;
using std::chrono::milliseconds;

constexpr std::uint16_t kGamePort = 4311;

/// A folder in the temp directory with a fake `aylo-runtime` (a shell script) in it.
struct FakeRuntime {
    std::filesystem::path directory;
    std::filesystem::path runtime;

    explicit FakeRuntime(const std::string& body) {
        static int counter = 0;
        directory = std::filesystem::temp_directory_path() /
                    ("aylo-online-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
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

/// Calls @p step until @p done, at most five seconds.
bool waitFor(const std::function<void()>& step, const std::function<bool()>& done) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (std::chrono::steady_clock::now() < deadline) {
        step();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(milliseconds{5});
    }
    return false;
}

server::ServerOptions optionsOf(const FakeRuntime& fake) {
    return server::ServerOptions{
        .gameDirectory = fake.directory, .port = kGamePort, .extraArgs = {}, .runtime = fake.runtime};
}

/// What the editor asked the engine for.
struct Rebuilds {
    std::vector<std::pair<RunMode, std::vector<std::string>>> calls;

    [[nodiscard]] play::OnlinePlay::Rebuild recorder() {
        return [this](const RunMode mode, std::vector<std::string> args) { calls.emplace_back(mode, std::move(args)); };
    }
};

/// A fake server that answers on the admin channel it is given, so the supervisor becomes kRunning.
class LiveServer {
  public:
    LiveServer() : _fake{"echo \"$@\"\nsleep 30"} {}

    [[nodiscard]] const FakeRuntime& fake() const { return _fake; }

    /// Starts the supervisor's server and opens the admin channel the fake server was told about.
    void bringUp(server::ServerSupervisor& supervisor) {
        ASSERT_TRUE(supervisor.start(optionsOf(_fake)).has_value());
        std::string port;
        std::string token;
        ASSERT_TRUE(waitFor([&] { supervisor.update(); },
                            [&] {
                                for (const editor::console::Line& line : supervisor.log().getLines()) {
                                    const auto at = line.text.find("--admin-port ");
                                    if (at == std::string::npos) {
                                        continue;
                                    }
                                    const auto words = editor::process::splitArguments(line.text.substr(at));
                                    port = words.at(1);
                                    token = words.at(3);
                                    return true;
                                }
                                return false;
                            }));
        network::NetworkSettings settings;
        settings.port = 0;
        _net = std::make_unique<network::server::NetServer>(settings);
        _admin = std::make_unique<admin::AdminServer>(static_cast<std::uint16_t>(std::stoi(port)), token,
                                                      network::Clock::now());
    }

    /// Lets the admin channel answer once.
    void serve() {
        _net->poll(network::Clock::now());
        _admin->poll(network::Clock::now(), *_net, admin::GameStatus{});
    }

    /// Runs the supervisor until it answers.
    [[nodiscard]] bool untilRunning(server::ServerSupervisor& supervisor) {
        return waitFor(
            [&] {
                serve();
                supervisor.update();
            },
            [&] { return supervisor.status() == server::ServerStatus::kRunning; });
    }

  private:
    FakeRuntime _fake;
    std::unique_ptr<network::server::NetServer> _net;
    std::unique_ptr<admin::AdminServer> _admin;
};

/// An App with the drawing plugins over the fake backend and the editor, in @p mode, sharing @p session.
class EditorApp {
  public:
    EditorApp(const RunMode mode, std::shared_ptr<editor::EditorSession> session)
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{mode} {
        rtype::test::plugins::addDrawingPlugins(_app, rtype::test::plugins::makeFakeRegistry(_state),
                                                rtype::test::plugins::makeFakeSettings());
        _app.addPlugin(rtype::plugins::capsule::CapsulePlugin{});
        _app.addPlugin(editor::EditorPlugin{editor::EditorOptions{
            .layoutFile = std::filesystem::path{}, .captureOutput = false, .session = std::move(session)}});
    }
    ~EditorApp() = default;
    EditorApp(const EditorApp& other) = delete;
    EditorApp& operator=(const EditorApp& other) = delete;
    EditorApp(EditorApp&& other) = delete;
    EditorApp& operator=(EditorApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;
    rtype::ecs::App _app;
};
}  // namespace

TEST(EditorOnlinePlay, StartsStandaloneAndChoosesTheModeOnlyWhileIdle) {
    play::OnlinePlay online;
    EXPECT_EQ(online.mode(), play::PlayMode::kStandalone);
    EXPECT_EQ(online.stage(), play::OnlineStage::kIdle);
    online.setMode(play::PlayMode::kOnline);
    EXPECT_EQ(online.mode(), play::PlayMode::kOnline);
}

TEST(EditorOnlinePlay, ReportsAServerThatCannotStart) {
    Rebuilds rebuilds;
    play::OnlinePlay online;
    online.setRebuild(rebuilds.recorder(), {"--editor"});
    server::ServerSupervisor supervisor;
    server::ServerOptions options;
    options.runtime = "/no/such/aylo-runtime";
    options.gameDirectory = "game";

    const auto started = online.begin(supervisor, options);

    EXPECT_FALSE(started.has_value());
    EXPECT_EQ(online.stage(), play::OnlineStage::kIdle);
    EXPECT_FALSE(online.message().empty());
    EXPECT_TRUE(rebuilds.calls.empty());
}

TEST(EditorOnlinePlay, WaitsForTheServerThenRebuildsAsAClientOfIt) {
    Rebuilds rebuilds;
    play::OnlinePlay online;
    online.setRebuild(rebuilds.recorder(), {"--editor"});
    online.setMode(play::PlayMode::kOnline);
    server::ServerSupervisor supervisor;
    LiveServer live;
    ASSERT_NO_FATAL_FAILURE(live.bringUp(supervisor));

    ASSERT_TRUE(online.begin(supervisor, optionsOf(live.fake())).has_value());
    EXPECT_EQ(online.stage(), play::OnlineStage::kStartingServer);
    online.update(supervisor);  // the server has not answered yet
    EXPECT_TRUE(rebuilds.calls.empty());
    online.setMode(play::PlayMode::kStandalone);
    EXPECT_EQ(online.mode(), play::PlayMode::kOnline);  // locked while Play is under way

    ASSERT_TRUE(live.untilRunning(supervisor));
    online.update(supervisor);

    ASSERT_EQ(rebuilds.calls.size(), 1U);
    EXPECT_EQ(rebuilds.calls[0].first, RunMode::kClient);
    const std::vector<std::string> expected{"--editor", "--host", "127.0.0.1", "--port", std::to_string(kGamePort)};
    EXPECT_EQ(rebuilds.calls[0].second, expected);
    EXPECT_EQ(online.stage(), play::OnlineStage::kConnecting);
    EXPECT_TRUE(online.isActive());

    online.update(supervisor);  // asked once only
    EXPECT_EQ(rebuilds.calls.size(), 1U);
    online.adopt();
    EXPECT_EQ(online.stage(), play::OnlineStage::kOnline);
}

TEST(EditorOnlinePlay, StopGoesBackToStandaloneAndLeavesTheServerRunning) {
    Rebuilds rebuilds;
    play::OnlinePlay online;
    online.setRebuild(rebuilds.recorder(), {"--editor"});
    server::ServerSupervisor supervisor;
    LiveServer live;
    ASSERT_NO_FATAL_FAILURE(live.bringUp(supervisor));
    ASSERT_TRUE(online.begin(supervisor, optionsOf(live.fake())).has_value());
    ASSERT_TRUE(live.untilRunning(supervisor));
    online.update(supervisor);
    online.adopt();

    online.end();

    ASSERT_EQ(rebuilds.calls.size(), 2U);
    EXPECT_EQ(rebuilds.calls[1].first, RunMode::kStandalone);
    EXPECT_EQ(rebuilds.calls[1].second, std::vector<std::string>{"--editor"});
    EXPECT_EQ(online.stage(), play::OnlineStage::kIdle);
    EXPECT_EQ(supervisor.status(), server::ServerStatus::kRunning);
}

TEST(EditorOnlinePlay, UsesAServerThatAlreadyRunsInsteadOfStartingAnother) {
    Rebuilds rebuilds;
    play::OnlinePlay online;
    online.setRebuild(rebuilds.recorder(), {});
    server::ServerSupervisor supervisor;
    LiveServer live;
    ASSERT_NO_FATAL_FAILURE(live.bringUp(supervisor));
    const std::uint16_t adminPort = supervisor.adminPort();
    server::ServerOptions other = optionsOf(live.fake());
    other.port = 5999;  // ignored: the running server keeps its own

    ASSERT_TRUE(online.begin(supervisor, other).has_value());

    EXPECT_EQ(supervisor.adminPort(), adminPort);
    EXPECT_EQ(supervisor.options().port, kGamePort);
}

TEST(EditorOnlinePlay, GivesUpWhenTheServerStopsBeforeItIsJoined) {
    Rebuilds rebuilds;
    play::OnlinePlay online;
    online.setRebuild(rebuilds.recorder(), {});
    FakeRuntime fake{"exit 3"};
    server::ServerSupervisor supervisor;

    ASSERT_TRUE(online.begin(supervisor, optionsOf(fake)).has_value());
    ASSERT_TRUE(
        waitFor([&] { supervisor.update(); }, [&] { return supervisor.status() == server::ServerStatus::kExited; }));
    online.update(supervisor);

    EXPECT_EQ(online.stage(), play::OnlineStage::kIdle);
    EXPECT_FALSE(online.message().empty());
    EXPECT_TRUE(rebuilds.calls.empty());
}

TEST(EditorOnlinePlay, CancellingWhileTheServerStartsRebuildsNothing) {
    Rebuilds rebuilds;
    play::OnlinePlay online;
    online.setRebuild(rebuilds.recorder(), {});
    FakeRuntime fake{"sleep 30"};
    server::ServerSupervisor supervisor;
    ASSERT_TRUE(online.begin(supervisor, optionsOf(fake)).has_value());

    online.end();

    EXPECT_EQ(online.stage(), play::OnlineStage::kIdle);
    EXPECT_TRUE(rebuilds.calls.empty());
    EXPECT_NE(supervisor.status(), server::ServerStatus::kStopped);  // the server is the Server panel's
}

TEST(EditorOnlinePlay, RefusesToBeginWithoutAWayToRebuild) {
    play::OnlinePlay online;
    server::ServerSupervisor supervisor;
    EXPECT_FALSE(online.begin(supervisor, server::ServerOptions{}).has_value());
    EXPECT_EQ(online.stage(), play::OnlineStage::kIdle);
}

TEST(EditorPluginOnline, RunsTheGameAtOnceWhenItIsTheClientPlayOnlineAskedFor) {
    Rebuilds rebuilds;
    const auto session = std::make_shared<editor::EditorSession>(false);
    session->online.setRebuild(rebuilds.recorder(), {});
    LiveServer live;
    ASSERT_NO_FATAL_FAILURE(live.bringUp(*session->supervisor));
    ASSERT_TRUE(session->online.begin(*session->supervisor, optionsOf(live.fake())).has_value());
    ASSERT_TRUE(live.untilRunning(*session->supervisor));
    session->online.update(*session->supervisor);
    ASSERT_EQ(session->online.stage(), play::OnlineStage::kConnecting);
    session->reopenScene = "scenes/level.scene";

    EditorApp client{RunMode::kClient, session};

    EXPECT_EQ(session->online.stage(), play::OnlineStage::kOnline);
    EXPECT_FALSE(client.app().getSchedules().isPaused());
    EXPECT_EQ(session->reopenScene, "scenes/level.scene");  // kept for the way back
}

TEST(EditorPluginOnline, GivesUpOnlinePlayWhenTheEngineWentBackToAnotherMode) {
    Rebuilds rebuilds;
    const auto session = std::make_shared<editor::EditorSession>(false);
    session->online.setRebuild(rebuilds.recorder(), {});
    LiveServer live;
    ASSERT_NO_FATAL_FAILURE(live.bringUp(*session->supervisor));
    ASSERT_TRUE(session->online.begin(*session->supervisor, optionsOf(live.fake())).has_value());
    ASSERT_TRUE(live.untilRunning(*session->supervisor));
    session->online.update(*session->supervisor);

    EditorApp standalone{RunMode::kStandalone, session};  // the client could not be built

    EXPECT_EQ(session->online.stage(), play::OnlineStage::kIdle);
    EXPECT_FALSE(session->online.message().empty());
    EXPECT_TRUE(standalone.app().getSchedules().isPaused());  // editing again
    EXPECT_EQ(session->supervisor->status(), server::ServerStatus::kRunning);
}

TEST(EditorPluginOnline, KeepsTheChildServerAcrossTwoApps) {
    const auto session = std::make_shared<editor::EditorSession>(false);
    LiveServer live;
    ASSERT_NO_FATAL_FAILURE(live.bringUp(*session->supervisor));
    {
        EditorApp first{RunMode::kStandalone, session};
    }
    {
        EditorApp second{RunMode::kClient, session};
        EXPECT_TRUE(second.app().getSchedules().isPaused());
    }
    EXPECT_NE(session->supervisor->status(), server::ServerStatus::kStopped);
    EXPECT_NE(session->supervisor->status(), server::ServerStatus::kExited);
}

#endif  // _WIN32
