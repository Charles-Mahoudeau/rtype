/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHttpTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <format>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "LocalHttpServer.hpp"
#include "ScriptProbe.hpp"
#include "plugins/http/HttpPlugin.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
using namespace std::chrono_literals;
using rtype::test::plugins::LocalHttpServer;

/// @brief A standalone App with the HttpPlugin reaching 127.0.0.1 over plain HTTP, and a script host on it.
struct HttpScripts {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    HttpScripts() {
        rtype::plugins::http::HttpSettings settings;
        settings.allowedHosts = {"127.0.0.1"};
        settings.allowHttp = true;
        settings.timeout = 5.0;
        app.addPlugin(rtype::plugins::http::HttpPlugin{settings});
        app.getPluginRegistry().addScriptForms("test.Probe", rtype::tests::kProbeForms);
        auto created = rtype::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("http-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Runs frames until the script's `probe.done` is true, for a few seconds at most.
    void runUntilDone() {
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        while (std::chrono::steady_clock::now() < deadline) {
            app.runFrame();
            if (run("assert(probe.done)").empty()) {
                return;
            }
            std::this_thread::sleep_for(2ms);
        }
    }
};

/// @return A server answering "/echo" with the body it received and the rest with "ok".
LocalHttpServer::Handler echo() {
    return [](const LocalHttpServer::Received& received) -> LocalHttpServer::Reply {
        return {.body = received.path == "/echo" ? received.body : std::string{"ok"}};
    };
}
}  // namespace

TEST(ScriptHttp, ASystemSendsARequestAndReadsItsResponse) {
    const LocalHttpServer server{echo()};
    HttpScripts scripts;
    ASSERT_EQ(scripts.run(std::format(R"(
        probe.done = false
        local sent = nil
        engine.ecs.system("game.Ask", {{
            phase = "Update",
            side = "both",
            query = {{}},
            calls = {{ "rtype.http.request" }},
            events = {{ read = {{ "rtype.HttpResponse" }} }},
            run = function(ctx)
                if sent == nil then
                    sent = engine.http.request(ctx, {{
                        method = "POST",
                        url = "{}",
                        headers = {{ ["Content-Type"] = "application/json", ["X-Squad"] = "miners" }},
                        body = string.rep("a", 50000),
                    }})
                end
                for response in ctx.events.read("rtype.HttpResponse") do
                    assert(response.request == sent, "wrong request")
                    assert(response.status == 200 and response.ok, tostring(response.status))
                    assert(#response.body == 50000, tostring(#response.body))
                    assert(response.error == "")
                    probe.done = true
                end
            end,
        }})
    )",
                                      server.url("/echo"))),
              "");
    scripts.runUntilDone();
    EXPECT_EQ(scripts.run("assert(probe.done)"), "");
    const auto received = server.getReceived();
    ASSERT_EQ(received.size(), 1U);
    EXPECT_NE(received[0].headers.find("X-Squad: miners"), std::string::npos);
    EXPECT_NE(received[0].headers.find("Content-Type: application/json"), std::string::npos);
}

TEST(ScriptHttp, AScriptCanSendWhileItLoadsAndUseTheRawCall) {
    const LocalHttpServer server{echo()};
    HttpScripts scripts;
    ASSERT_EQ(scripts.run(std::format(R"(
        probe.done = false
        local first = engine.http.request(nil, {{ url = "{}" }})
        local second = engine.call("rtype.http.request", {{ url = "{}", headers = "X-Raw: yes" }})
        assert(first ~= second)
        local seen = 0
        engine.ecs.system("game.Read", {{
            phase = "Update",
            side = "both",
            query = {{}},
            events = {{ read = {{ "rtype.HttpResponse" }} }},
            run = function(ctx)
                for response in ctx.events.read("rtype.HttpResponse") do
                    assert(response.body == "ok")
                    seen += 1
                end
                probe.done = seen == 2
            end,
        }})
    )",
                                      server.url("/a"), server.url("/b"))),
              "");
    scripts.runUntilDone();
    EXPECT_EQ(scripts.run("assert(probe.done)"), "");
}

TEST(ScriptHttp, MistakesAreLuauErrorsNamingThem) {
    HttpScripts scripts;
    const auto expectError = [&scripts](const std::string& source, const std::string& message) {
        const std::string error = scripts.run(source);
        EXPECT_NE(error.find(message), std::string::npos) << "got: " << error;
    };
    expectError(R"(engine.http.request(nil, { url = "https://elsewhere.com/" }))", "not in http.allowedHosts");
    expectError(R"(engine.http.request(nil, { link = "x" }))", "engine.http.request: unexpected 'link'");
    expectError(R"(engine.http.request(nil, {}))", "engine.http.request: expected a 'url'");
    expectError(R"(engine.http.request(nil, { url = "http://127.0.0.1/", headers = { A = 1 } }))",
                "header 'A': must be a string");
    expectError(R"(engine.call("rtype.http.request", { url = "http://127.0.0.1/", method = "BREW" }))",
                "Unknown HTTP method 'BREW'");
}
