/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "core/system/ExecutablePath.hpp"

namespace rtype::editor::process {

std::filesystem::path runtimePath() {
#ifdef _WIN32
    constexpr const char* kName = "aylo-runtime.exe";
#else
    constexpr const char* kName = "aylo-runtime";
#endif
    return core::system::getExecutableDirectory() / kName;
}

std::vector<std::string> serverArguments(const std::filesystem::path& game, const std::uint16_t port,
                                         const std::uint16_t adminPort, const std::string& adminToken,
                                         const std::vector<std::string>& extra) {
    // The game folder first, and --server before another flag: a flag takes the word after it as its value.
    std::vector<std::string> args{game.string(), "--server", "--port", std::to_string(port)};
    if (adminPort != 0) {
        args.emplace_back("--admin-port");
        args.push_back(std::to_string(adminPort));
        args.emplace_back("--admin-token");
        args.push_back(adminToken);
    }
    args.insert(args.end(), extra.begin(), extra.end());
    return args;
}

std::vector<std::string> clientArguments(const std::filesystem::path& game, const std::string& host,
                                         const std::uint16_t port, const std::vector<std::string>& extra) {
    std::vector<std::string> args{game.string(), "--host", host, "--port", std::to_string(port)};
    args.insert(args.end(), extra.begin(), extra.end());
    return args;
}

std::vector<std::string> splitArguments(const std::string& text) {
    std::vector<std::string> args;
    std::string current;
    bool quoted = false;
    bool started = false;
    for (const char c : text) {
        if (c == '"') {
            quoted = !quoted;
            started = true;
        } else if (c == ' ' && !quoted) {
            if (started) {
                args.push_back(std::move(current));
                current.clear();
                started = false;
            }
        } else {
            current += c;
            started = true;
        }
    }
    if (started) {
        args.push_back(std::move(current));
    }
    return args;
}

}  // namespace rtype::editor::process
