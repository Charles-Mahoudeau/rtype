/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientLauncher
*/

#include "ClientLauncher.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "editor/console/ConsoleLog.hpp"
#include "editor/process/ChildProcess.hpp"
#include "editor/process/Runtime.hpp"

namespace rtype::editor::process {

namespace {
constexpr std::chrono::milliseconds kGrace{1500};  ///< How long the clients get to end after a stop request.
constexpr std::chrono::milliseconds kPoll{5};
}  // namespace

ClientLauncher::ClientLauncher(std::shared_ptr<console::ConsoleLog> sink) : _sink{std::move(sink)} {}

ClientLauncher::~ClientLauncher() {
    try {
        killAll();
    } catch (...) {  // NOLINT(bugprone-empty-catch) only the logging can throw; the processes are stopped by their
                     // destructors anyway
    }
}

tl::expected<ChildProcess, std::string> ClientLauncher::spawn(const ClientOptions& options,
                                                              const std::vector<std::string>& extraArgs) {
    if (options.gameDirectory.empty() || !std::filesystem::is_directory(options.gameDirectory)) {
        return tl::make_unexpected(std::format("'{}' is not a game folder", options.gameDirectory.string()));
    }
    if (options.port == 0) {
        return tl::make_unexpected("the port must be from 1 to 65535");
    }
    const std::filesystem::path runtime = options.runtime.empty() ? runtimePath() : options.runtime;
    if (!std::filesystem::exists(runtime)) {
        return tl::make_unexpected(std::format("aylo-runtime is not at '{}'", runtime.string()));
    }
    return ChildProcess::spawn(ProcessSpec{
        .program = runtime,
        .args = clientArguments(options.gameDirectory, options.host, options.port, extraArgs),
        .workingDirectory = {},
    });
}

void ClientLauncher::drain(Client& client) {
    if (client.process) {
        if (const std::string text = client.process->drainOutput(); !text.empty()) {
            client.log->write(text);
        }
    }
    const std::size_t added = client.log->getAdded();
    if (!_sink || added <= client.forwarded) {
        client.forwarded = added;
        return;
    }
    const std::vector<console::Line> lines = client.log->getLines();
    const std::size_t fresh = std::min(added - client.forwarded, lines.size());
    for (auto line = lines.end() - static_cast<std::ptrdiff_t>(fresh); line != lines.end(); ++line) {
        _sink->addLine(std::format("[client {}] {}", client.number, line->text));
    }
    client.forwarded = added;
}

void ClientLauncher::note(Client& client, const std::string& text) {
    client.log->addLine(text);
    drain(client);
}

tl::expected<int, std::string> ClientLauncher::launch(const ClientOptions& options,
                                                      const std::vector<std::string>& extraArgs) {
    auto child = spawn(options, extraArgs);
    if (!child) {
        return tl::make_unexpected(std::move(child.error()));
    }
    Client& client = _clients.emplace_back();
    client.number = _next++;
    client.args = extraArgs;
    client.log = std::make_unique<console::ConsoleLog>();
    client.process.emplace(std::move(*child));
    note(client, std::format("Started (process {})", client.process->pid()));
    return client.number;
}

void ClientLauncher::kill(const int number) {
    for (Client& client : _clients) {
        if (client.number == number && client.process) {
            client.process->stop();
            drain(client);
            client.exitCode = client.process->exitCode();
            client.process.reset();
            note(client, "Stopped");
        }
    }
}

void ClientLauncher::killAll() {
    for (Client& client : _clients) {
        if (client.process) {
            client.process->requestStop();
        }
    }
    const auto deadline = std::chrono::steady_clock::now() + kGrace;
    for (Client& client : _clients) {
        if (client.process) {
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::max(deadline - std::chrono::steady_clock::now(), std::chrono::steady_clock::duration::zero()));
            if (!client.process->waitFor(left)) {
                client.process->kill();
            }
            drain(client);
            client.exitCode = client.process->exitCode();
            client.process.reset();
            note(client, "Stopped");
        }
    }
}

tl::expected<void, std::string> ClientLauncher::restartAll(const ClientOptions& options) {
    killAll();
    std::optional<std::string> failure;
    for (Client& client : _clients) {
        auto child = spawn(options, client.args);
        if (!child) {
            note(client, std::format("Error: {}", child.error()));
            if (!failure) {
                failure = std::move(child.error());
            }
            continue;
        }
        client.exitCode.reset();
        client.process.emplace(std::move(*child));
        note(client, std::format("Started again (process {})", client.process->pid()));
    }
    if (failure) {
        return tl::make_unexpected(std::move(*failure));
    }
    return {};
}

void ClientLauncher::removeEnded() {
    std::erase_if(_clients, [](const Client& client) { return !client.process; });
}

void ClientLauncher::update() {
    for (Client& client : _clients) {
        if (!client.process) {
            continue;
        }
        if (!client.process->isRunning()) {
            drain(client);
            client.exitCode = client.process->exitCode();
            client.process.reset();
            note(client, std::format("Ended with code {}", client.exitCode.value_or(0)));
        } else {
            drain(client);
        }
    }
}

std::vector<ClientInfo> ClientLauncher::list() const {
    std::vector<ClientInfo> infos;
    infos.reserve(_clients.size());
    for (const Client& client : _clients) {
        infos.push_back(ClientInfo{
            .number = client.number,
            .pid = client.process ? client.process->pid() : 0,
            .running = client.process.has_value(),
            .exitCode = client.exitCode,
            .args = client.args,
        });
    }
    return infos;
}

std::size_t ClientLauncher::runningCount() const noexcept {
    return static_cast<std::size_t>(
        std::ranges::count_if(_clients, [](const Client& client) { return client.process.has_value(); }));
}

}  // namespace rtype::editor::process
