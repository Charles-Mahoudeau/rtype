/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HttpTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "LocalHttpServer.hpp"
#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "plugins/http/HttpCalls.hpp"
#include "plugins/http/HttpClient.hpp"
#include "plugins/http/HttpEvents.hpp"
#include "plugins/http/HttpPlugin.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "plugins/http/exceptions/HttpException.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"

namespace {
using rtype::core::config::Settings;
using rtype::core::config::exceptions::SettingsException;
using rtype::plugins::http::HttpClient;
using rtype::plugins::http::HttpRequest;
using rtype::plugins::http::HttpResult;
using rtype::plugins::http::HttpSettings;
using rtype::plugins::http::RequestId;
using rtype::plugins::http::exceptions::HttpException;
using rtype::test::plugins::LocalHttpServer;
using namespace std::chrono_literals;

/// @return Settings reaching the local server over plain HTTP.
HttpSettings localSettings() {
    HttpSettings settings;
    settings.allowedHosts = {"127.0.0.1"};
    settings.allowHttp = true;
    settings.timeout = 5.0;
    return settings;
}

/// @return A server answering every request: "/echo" with its body, "/status/<n>" with that status, "/big" with 4 KiB,
/// "/slow" after a second, anything else with "ok".
LocalHttpServer::Handler routes() {
    return [](const LocalHttpServer::Received& received) -> LocalHttpServer::Reply {
        if (received.path == "/echo") {
            return {.body = received.body};
        }
        if (received.path.starts_with("/status/")) {
            return {.status = std::stoi(received.path.substr(8)), .body = "status"};
        }
        if (received.path == "/big") {
            return {.body = std::string(4096, 'b')};
        }
        if (received.path == "/slow") {
            return {.body = "late", .delay = 1000ms};
        }
        return {.body = "ok"};
    };
}

/// @return The results of @p client until @p count requests ended, polling for at most a few seconds.
std::vector<HttpResult> waitFor(HttpClient& client, const std::size_t count) {
    std::vector<HttpResult> results;
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (results.size() < count && std::chrono::steady_clock::now() < deadline) {
        for (HttpResult& result : client.poll()) {
            results.push_back(std::move(result));
        }
        std::this_thread::sleep_for(2ms);
    }
    return results;
}

/// @brief Sets the environment variable @p name to @p value for the test.
void setEnvironment(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);  // NOLINT(concurrency-mt-unsafe): the tests run on one thread
#endif
}

/// @return A section holding @p key only.
Settings sectionWith(const char* key, Settings::Value value) {
    Settings settings;
    settings.set(key, std::move(value));
    return settings;
}
}  // namespace

// ---- settings ----

TEST(HttpSettings, KeepsSafeDefaults) {
    const HttpSettings settings = HttpSettings::fromSettings(Settings{});
    EXPECT_TRUE(settings.allowedHosts.empty());
    EXPECT_FALSE(settings.allowHttp);
    EXPECT_EQ(settings.maxConcurrent, 8U);
    EXPECT_DOUBLE_EQ(settings.timeout, 30.0);
    EXPECT_EQ(settings.maxResponseBytes, 1024U * 1024U);
    EXPECT_TRUE(settings.secrets.empty());
}

TEST(HttpSettings, ReadsEveryKeyAndLowersTheHosts) {
    Settings section;
    section.set("allowedHosts", std::vector<std::string>{"API.Example.com"});
    section.set("allowHttp", true);
    section.set("maxConcurrent", 2.0);
    section.set("timeout", 4.5);
    section.set("maxResponseBytes", 100.0);
    section.set("secrets", std::vector<std::string>{"RUBIX_API_KEY"});
    const HttpSettings settings = HttpSettings::fromSettings(section);
    EXPECT_EQ(settings.allowedHosts, std::vector<std::string>{"api.example.com"});
    EXPECT_TRUE(settings.allowHttp);
    EXPECT_EQ(settings.maxConcurrent, 2U);
    EXPECT_DOUBLE_EQ(settings.timeout, 4.5);
    EXPECT_EQ(settings.maxResponseBytes, 100U);
    EXPECT_EQ(settings.secrets, std::vector<std::string>{"RUBIX_API_KEY"});
}

TEST(HttpSettings, RejectsInvalidValues) {
    EXPECT_THROW(static_cast<void>(HttpSettings::fromSettings(sectionWith("hosts", 1.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(HttpSettings::fromSettings(
                     sectionWith("allowedHosts", std::vector<std::string>{"https://api.example.com"}))),
                 SettingsException);
    EXPECT_THROW(static_cast<void>(HttpSettings::fromSettings(sectionWith("maxConcurrent", 0.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(HttpSettings::fromSettings(sectionWith("timeout", -1.0))), SettingsException);
    EXPECT_THROW(static_cast<void>(HttpSettings::fromSettings(sectionWith("maxResponseBytes", 1.5))),
                 SettingsException);
    EXPECT_THROW(
        static_cast<void>(HttpSettings::fromSettings(sectionWith("secrets", std::vector<std::string>{"NOT A NAME"}))),
        SettingsException);
}

// ---- client ----

TEST(HttpClient, GetsAResponseWithoutWaiting) {
    const LocalHttpServer server{routes()};
    HttpClient client{localSettings()};
    const RequestId id = client.send(HttpRequest{.url = server.url("/hello")});
    EXPECT_NE(id, 0U);
    EXPECT_EQ(client.getPendingCount(), 1U);

    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].request, id);
    EXPECT_EQ(results[0].status, 200);
    EXPECT_EQ(results[0].body, "ok");
    EXPECT_TRUE(results[0].error.empty());
    EXPECT_EQ(client.getPendingCount(), 0U);
}

TEST(HttpClient, SendsTheMethodTheHeadersAndALongBody) {
    const LocalHttpServer server{routes()};
    HttpClient client{localSettings()};
    const std::string body(100000, 'x');
    static_cast<void>(client.send(HttpRequest{.method = "post",
                                              .url = server.url("/echo"),
                                              .headers = "Content-Type: application/json\nX-Game: gridmind\n",
                                              .body = body}));
    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].body, body);
    const auto received = server.getReceived();
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0].method, "POST");
    EXPECT_NE(received[0].headers.find("X-Game: gridmind"), std::string::npos);
    EXPECT_NE(received[0].headers.find("Content-Type: application/json"), std::string::npos);
}

TEST(HttpClient, AnErrorStatusIsAResponseNotAnError) {
    const LocalHttpServer server{routes()};
    HttpClient client{localSettings()};
    static_cast<void>(client.send(HttpRequest{.url = server.url("/status/404")}));
    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].status, 404);
    EXPECT_TRUE(results[0].error.empty());
}

TEST(HttpClient, ANetworkFailureEndsWithAnError) {
    std::string url;
    {
        const LocalHttpServer server{routes()};
        url = server.url("/gone");
    }  // nothing listens on its port any more
    HttpClient client{localSettings()};
    static_cast<void>(client.send(HttpRequest{.url = url}));
    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].status, 0);
    EXPECT_FALSE(results[0].error.empty());
}

TEST(HttpClient, StopsAResponseLongerThanTheLimit) {
    const LocalHttpServer server{routes()};
    HttpSettings settings = localSettings();
    settings.maxResponseBytes = 1000;
    HttpClient client{settings};
    static_cast<void>(client.send(HttpRequest{.url = server.url("/big")}));
    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].status, 0);
    EXPECT_NE(results[0].error.find("maxResponseBytes"), std::string::npos) << results[0].error;
}

TEST(HttpClient, TimesOut) {
    const LocalHttpServer server{routes()};
    HttpClient client{localSettings()};
    static_cast<void>(client.send(HttpRequest{.url = server.url("/slow"), .timeout = 0.2}));
    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].status, 0);
    EXPECT_FALSE(results[0].error.empty());
}

TEST(HttpClient, RefusesWhatIsNotAllowed) {
    HttpClient client{localSettings()};
    EXPECT_THROW(static_cast<void>(client.send(HttpRequest{.url = "http://example.com/"})), HttpException);
    EXPECT_THROW(static_cast<void>(client.send(HttpRequest{.url = "not a url"})), HttpException);
    EXPECT_THROW(static_cast<void>(client.send(HttpRequest{.url = "ftp://127.0.0.1/"})), HttpException);
    EXPECT_THROW(static_cast<void>(client.send(HttpRequest{.method = "BREW", .url = "http://127.0.0.1/"})),
                 HttpException);
    EXPECT_THROW(static_cast<void>(client.send(HttpRequest{.url = "http://127.0.0.1/", .body = "x"})), HttpException);
    EXPECT_THROW(static_cast<void>(client.send(HttpRequest{.url = "http://127.0.0.1/", .headers = "no colon"})),
                 HttpException);

    HttpSettings strict = localSettings();
    strict.allowHttp = false;
    HttpClient https{strict};
    EXPECT_THROW(static_cast<void>(https.send(HttpRequest{.url = "http://127.0.0.1/"})), HttpException);
    EXPECT_EQ(client.getPendingCount() + https.getPendingCount(), 0U);
}

TEST(HttpClient, PutsAListedSecretInAHeaderOnly) {
    const LocalHttpServer server{routes()};
    HttpSettings settings = localSettings();
    settings.secrets = {"AYLO_TEST_SECRET", "AYLO_TEST_UNSET"};
    setEnvironment("AYLO_TEST_SECRET", "s3cr3t");
    HttpClient client{settings};
    static_cast<void>(client.send(
        HttpRequest{.url = server.url("/hello"), .headers = "Authorization: Bearer {{env:AYLO_TEST_SECRET}}"}));
    ASSERT_EQ(waitFor(client, 1).size(), 1U);
    const auto received = server.getReceived();
    ASSERT_EQ(received.size(), 1U);
    EXPECT_NE(received[0].headers.find("Authorization: Bearer s3cr3t"), std::string::npos);

    EXPECT_THROW(
        static_cast<void>(client.send(HttpRequest{.url = server.url("/"), .headers = "Authorization: {{env:HOME}}"})),
        HttpException);
    EXPECT_THROW(static_cast<void>(client.send(
                     HttpRequest{.url = server.url("/"), .headers = "Authorization: {{env:AYLO_TEST_UNSET}}"})),
                 HttpException);
}

TEST(HttpClient, QueuesWhatIsOverTheLimitAndCancels) {
    const LocalHttpServer server{routes()};
    HttpSettings settings = localSettings();
    settings.maxConcurrent = 1;
    HttpClient client{settings};
    const RequestId first = client.send(HttpRequest{.url = server.url("/a")});
    const RequestId second = client.send(HttpRequest{.url = server.url("/b")});
    const RequestId third = client.send(HttpRequest{.url = server.url("/c")});
    EXPECT_EQ(client.getPendingCount(), 3U);
    EXPECT_TRUE(client.cancel(second));
    EXPECT_FALSE(client.cancel(second));
    EXPECT_FALSE(client.cancel(9999));

    const std::vector<HttpResult> results = waitFor(client, 2);
    ASSERT_EQ(results.size(), 2U);
    EXPECT_EQ(results[0].request, first);
    EXPECT_EQ(results[1].request, third);
    const auto received = server.getReceived();
    ASSERT_EQ(received.size(), 2U);
    EXPECT_EQ(received[1].path, "/c");
}

// ---- plugin ----

TEST(HttpPlugin, SendsAnHttpResponseForARequestOfItsCall) {
    const LocalHttpServer server{routes()};
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::http::HttpPlugin{localSettings()});
    rtype::ecs::World& world = app.getWorld();
    ASSERT_NE(world.getCalls().findByName(rtype::plugins::http::kRequestCall), nullptr);
    ASSERT_NE(world.getCalls().findByName(rtype::plugins::http::kCancelCall), nullptr);

    const RequestId id = world.getResources().getMut<HttpClient>()->send(HttpRequest{.url = server.url("/status/201")});
    rtype::ecs::EventCursor cursor;
    std::vector<rtype::plugins::http::HttpResponse> responses;
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (responses.empty() && std::chrono::steady_clock::now() < deadline) {
        app.runFrame();
        for (const auto& response : world.getEvents().read<rtype::plugins::http::HttpResponse>(cursor)) {
            responses.push_back(response);
        }
        std::this_thread::sleep_for(2ms);
    }
    ASSERT_EQ(responses.size(), 1U);
    EXPECT_EQ(responses[0].request, id);
    EXPECT_EQ(responses[0].status, 201);
    EXPECT_TRUE(responses[0].ok);
    EXPECT_EQ(responses[0].body, "status");
}

// Reaches the internet, so CI does not run it: `plugins-tests --gtest_also_run_disabled_tests
// --gtest_filter=HttpClient.DISABLED_*` checks that this build speaks TLS and trusts the system's certificates.
TEST(HttpClient, DISABLED_ReachesARealHttpsServer) {
    HttpSettings settings;
    settings.allowedHosts = {"example.com"};
    HttpClient client{settings};
    static_cast<void>(client.send(HttpRequest{.url = "https://example.com/"}));
    const std::vector<HttpResult> results = waitFor(client, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].status, 200) << results[0].error;
    EXPECT_FALSE(results[0].body.empty());
}
