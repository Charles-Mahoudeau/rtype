/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-runtime: runs a game in `game/` (or `--game <directory>`), with the plugins and backends its project.luau names,
// loaded from their libraries. It runs the client side by default, and the headless server side with `--server`. Every
// other flag goes to project.luau as `engine.args`.
//   aylo-runtime [--server] [--game <directory>] [--<flag> <value>]...

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"

namespace {
constexpr int kErrorCode = 84;                        ///< What the program returns when it cannot run the game.
constexpr std::string_view kServerFlag = "--server";  ///< Runs the server side, headless, instead of the client.

/// @brief Takes the server flag out of @p args.
/// @param args The arguments, without the program name.
/// @return Whether the flag was there: the engine and the game never see it.
bool takeServerFlag(std::vector<std::string>& args) {
    const auto [first, last] = std::ranges::remove(args, kServerFlag);
    const bool found = first != last;

    args.erase(first, last);
    return found;
}
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span rawArgs{argv, static_cast<std::size_t>(argc)};
        std::vector<std::string> args;
        if (!rawArgs.empty()) {  // Some platforms start a program with no argv[0].
            args.assign(rawArgs.begin() + 1, rawArgs.end());
        }
        const bool server = takeServerFlag(args);
        rtype::engine::Engine engine{{
            .mode = server ? rtype::ecs::RunMode::kServer : rtype::ecs::RunMode::kClient,
            .args = std::move(args),
        }};

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
