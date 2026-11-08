/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-runtime: runs a game in `game/` (or the <directory> given first), with the plugins and backends its project.luau
// names, loaded from their libraries. It runs the client side by default, and the headless server side with `--server`.
// Every other flag goes to project.luau as `engine.args`. The editor is another program, `aylo-editor`.
//   aylo-runtime [--server] [--ticks <n>] [<directory>] [--<flag> <value>]...
//   aylo-runtime --version

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/library/LibraryEntry.hpp"
#include "core/version/Version.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/CommandLine.hpp"
#include "rtype/engine/Engine.hpp"

namespace {
constexpr int kErrorCode = 84;                          ///< What the program returns when it cannot run the game.
constexpr std::string_view kServerFlag = "--server";    ///< Runs the server side, headless, instead of the client.
constexpr std::string_view kVersionFlag = "--version";  ///< Prints the version of the engine and stops.

// What the build says about itself: xmake gives the version and the commit (xmake/modules/engine_version.lua).
// NOLINTBEGIN(cppcoreguidelines-macro-usage): the build passes them as macros; these are the defaults without it.
#ifndef RTYPE_ENGINE_VERSION
#define RTYPE_ENGINE_VERSION "0.0.0"
#endif
#ifndef RTYPE_ENGINE_COMMIT
#define RTYPE_ENGINE_COMMIT ""
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

/// @brief Prints what `--version` says: the version, the commit, the toolchain and the library ABI version.
void printVersion() {
    const rtype::core::version::VersionInfo info{
        .version = RTYPE_ENGINE_VERSION,
        .commit = RTYPE_ENGINE_COMMIT,
        .toolchain = RTYPE_LIBRARY_TOOLCHAIN,
        .libraryAbiVersion = rtype::core::library::kLibraryAbiVersion,
    };

    std::cout << rtype::core::version::describe(info);
}

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
        if (std::ranges::find(args, kVersionFlag) != args.end()) {
            printVersion();
            return 0;
        }
        const bool server = takeServerFlag(args);
        const std::optional<std::size_t> ticks = rtype::engine::takeFrameLimit(args);
        rtype::engine::Engine engine{{
            .mode = server ? rtype::ecs::RunMode::kServer : rtype::ecs::RunMode::kClient,
            .args = std::move(args),
        }};

        return engine.run(ticks);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
