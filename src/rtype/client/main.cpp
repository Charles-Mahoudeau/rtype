/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// r-type_client: runs the client side of the game in `game/` (or `--game <directory>`), with the plugins and backends
// its project.luau names, loaded from their libraries. Every other flag goes to project.luau as `engine.args`.
//   r-type_client [--game <directory>] [--<flag> <value>]...

#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"

namespace {
constexpr int kErrorCode = 84;  ///< What the program returns when it cannot run the game.
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span args{argv, static_cast<std::size_t>(argc)};
        rtype::engine::Engine engine{{
            .mode = rtype::ecs::RunMode::kClient,
            .args = std::vector<std::string>{args.begin() + 1, args.end()},
        }};

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
