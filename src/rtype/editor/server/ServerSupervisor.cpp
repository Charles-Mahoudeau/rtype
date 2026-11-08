/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerSupervisor
*/

#include "ServerSupervisor.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "editor/console/ConsoleLog.hpp"
#include "editor/process/ChildProcess.hpp"
#include "editor/process/Runtime.hpp"
#include "editor/server/AdminSession.hpp"
#include "rtype/network/io/UdpSocket.hpp"

namespace rtype::editor::server {

namespace {
constexpr std::string_view kPrefix = "[server] ";
}  // namespace

ServerSupervisor::ServerSupervisor(std::shared_ptr<console::ConsoleLog> sink) : _sink{std::move(sink)} {}

std::uint16_t ServerSupervisor::findFreePort() {
    try {
        const network::io::UdpSocket probe{0, true};
        return probe.localPort();
    } catch (const std::exception&) {
        return 0;
    }
}

std::string ServerSupervisor::makeToken() {
    std::random_device device;
    const std::uint64_t high = (static_cast<std::uint64_t>(device()) << 32U) | device();
    const std::uint64_t low = (static_cast<std::uint64_t>(device()) << 32U) | device();
    return std::format("{:016x}{:016x}", high, low);
}

void ServerSupervisor::note(const std::string& text) {
    _log.addLine(text);
    drainOutput();  // forwards it
}

void ServerSupervisor::drainOutput() {
    if (_process) {
        if (const std::string text = _process->drainOutput(); !text.empty()) {
            _log.write(text);
        }
    }
    if (!_sink) {
        return;
    }
    const std::size_t added = _log.getAdded();
    if (added <= _forwarded) {
        return;
    }
    const std::vector<console::Line> lines = _log.getLines();
    const std::size_t fresh = std::min(added - _forwarded, lines.size());
    for (auto line = lines.end() - static_cast<std::ptrdiff_t>(fresh); line != lines.end(); ++line) {
        _sink->addLine(std::string{kPrefix} + line->text);
    }
    _forwarded = added;
}

std::expected<void, std::string> ServerSupervisor::start(ServerOptions options) {
    if (_status == ServerStatus::kStarting || _status == ServerStatus::kRunning) {
        return std::unexpected("the server is already running");
    }
    if (options.gameDirectory.empty() || !std::filesystem::is_directory(options.gameDirectory)) {
        return std::unexpected(std::format("'{}' is not a game folder", options.gameDirectory.string()));
    }
    if (options.port == 0) {
        return std::unexpected("the port must be from 1 to 65535");
    }
    if (options.runtime.empty()) {
        options.runtime = process::runtimePath();
    }
    if (!std::filesystem::exists(options.runtime)) {
        return std::unexpected(std::format("aylo-runtime is not at '{}'", options.runtime.string()));
    }
    const std::uint16_t adminPort = findFreePort();
    if (adminPort == 0) {
        return std::unexpected("no free local port for the admin channel");
    }
    const std::string token = makeToken();

    const process::ProcessSpec spec{
        .program = options.runtime,
        .args = process::serverArguments(options.gameDirectory, options.port, adminPort, token, options.extraArgs),
        .workingDirectory = {},
    };
    auto child = process::ChildProcess::spawn(spec);
    if (!child) {
        note(std::format("Error: {}", child.error()));
        return std::unexpected(std::move(child.error()));
    }
    std::unique_ptr<AdminSession> session;
    try {
        session = std::make_unique<AdminSession>(adminPort, token);
    } catch (const std::exception& error) {
        return std::unexpected(std::format("cannot open the admin socket: {}", error.what()));
    }
    _options = std::move(options);
    _adminPort = adminPort;
    _exitCode.reset();
    _process.emplace(std::move(*child));
    _session = std::move(session);
    _status = ServerStatus::kStarting;
    note(std::format("Started the server (process {}) on port {}", _process->pid(), _options.port));
    return {};
}

void ServerSupervisor::stop() {
    if (_process) {
        process::ChildProcess& process = *_process;
        process.stop();
        drainOutput();
        if (process.exitCode()) {
            note("The server stopped");
        }
        _process.reset();
    }
    _session.reset();
    _adminPort = 0;
    _exitCode.reset();
    _status = ServerStatus::kStopped;
}

std::expected<void, std::string> ServerSupervisor::restart() {
    ServerOptions options = _options;
    stop();
    return start(std::move(options));
}

void ServerSupervisor::update(const Clock::time_point now) {
    if (!_process) {
        return;
    }
    process::ChildProcess& process = *_process;
    if (!process.isRunning()) {
        drainOutput();
        _exitCode = process.exitCode();
        _process.reset();
        _session.reset();
        _adminPort = 0;
        _status = ServerStatus::kExited;
        note(std::format("The server ended with code {}", _exitCode.value_or(0)));
        return;
    }
    drainOutput();
    _session->update(now);
    _status = _session->isConnected() && !_session->isLost() ? ServerStatus::kRunning : ServerStatus::kStarting;
}

}  // namespace rtype::editor::server
