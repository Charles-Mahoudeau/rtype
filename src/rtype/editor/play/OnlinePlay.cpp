/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** OnlinePlay
*/

#include "OnlinePlay.hpp"

#include <string>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "editor/server/ServerSupervisor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

namespace rtype::editor::play {

namespace {
constexpr const char* kLoopback = "127.0.0.1";  ///< Where the child server listens, seen from the editor.
}  // namespace

void OnlinePlay::setRebuild(Rebuild rebuild, std::vector<std::string> baseArgs) {
    _rebuild = std::move(rebuild);
    _baseArgs = std::move(baseArgs);
}

void OnlinePlay::setMode(const PlayMode mode) noexcept {
    if (_stage == OnlineStage::kIdle) {
        _mode = mode;
    }
}

tl::expected<void, std::string> OnlinePlay::begin(server::ServerSupervisor& supervisor, server::ServerOptions options) {
    if (_stage != OnlineStage::kIdle) {
        return {};
    }
    _message.clear();
    if (!_rebuild) {
        _message = "Play online needs the program to rebuild the game as a client";
        return tl::make_unexpected(_message);
    }
    const server::ServerStatus status = supervisor.status();
    if (status == server::ServerStatus::kStopped || status == server::ServerStatus::kExited) {
        if (auto started = supervisor.start(std::move(options)); !started) {
            _message = started.error();
            return tl::make_unexpected(_message);
        }
    }
    _stage = OnlineStage::kStartingServer;
    return {};
}

void OnlinePlay::update(server::ServerSupervisor& supervisor) {
    if (_stage == OnlineStage::kStartingServer) {
        switch (supervisor.status()) {
            case server::ServerStatus::kRunning:
                _stage = OnlineStage::kConnecting;
                _rebuild(ecs::RunMode::kClient, [&] {
                    std::vector<std::string> args = _baseArgs;
                    args.insert(args.end(), {"--host", kLoopback, "--port", std::to_string(supervisor.options().port)});
                    return args;
                }());
                break;
            case server::ServerStatus::kStopped:
            case server::ServerStatus::kExited:
                _stage = OnlineStage::kIdle;
                _message = "The server stopped before it could be joined";
                break;
            case server::ServerStatus::kStarting:
                break;
        }
    } else if (_stage == OnlineStage::kOnline && supervisor.status() == server::ServerStatus::kExited) {
        _message = "The server stopped: Stop to go back to the scene";
    }
}

void OnlinePlay::adopt() noexcept {
    if (_stage == OnlineStage::kConnecting) {
        _stage = OnlineStage::kOnline;
    }
}

void OnlinePlay::fail(std::string reason) {
    _stage = OnlineStage::kIdle;
    _message = std::move(reason);
}

void OnlinePlay::end() {
    const OnlineStage before = std::exchange(_stage, OnlineStage::kIdle);
    _message.clear();
    if ((before == OnlineStage::kConnecting || before == OnlineStage::kOnline) && _rebuild) {
        _rebuild(ecs::RunMode::kStandalone, _baseArgs);
    }
}

}  // namespace rtype::editor::play
