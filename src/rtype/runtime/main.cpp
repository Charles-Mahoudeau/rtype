/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-runtime: runs a game in `game/` (or `--game <directory>`), with the plugins and backends its project.luau names,
// loaded from their libraries. It runs the client side by default, and the headless server side with `--server`. With
// `--editor`, it runs both sides in one world (no network) and adds the editor ("rtype.Editor") to the plugins: the
// game, with its server's gameplay, inside the editor's panels. Every other flag goes to project.luau as `engine.args`,
// `--editor` included, so a project can tell.
//   aylo-runtime [--server | --editor] [--game <directory>] [--<flag> <value>]...
//   aylo-runtime --version

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/library/LibraryEntry.hpp"
#include "core/version/Version.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"

namespace {
constexpr int kErrorCode = 84;                              ///< What the program returns when it cannot run the game.
constexpr std::string_view kServerFlag = "--server";        ///< Runs the server side, headless, instead of the client.
constexpr std::string_view kVersionFlag = "--version";      ///< Prints the version of the engine and stops.
constexpr std::string_view kEditorFlag = "--editor";        ///< Runs both sides in the editor instead of a client.
constexpr std::string_view kEditorPlugin = "rtype.Editor";  ///< The plugin `--editor` adds.

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
        const bool editor = std::ranges::find(args, kEditorFlag) != args.end();
        if (server && editor) {
            std::cerr << "Error: --server and --editor cannot go together: the editor runs both sides\n";
            return kErrorCode;
        }
        rtype::ecs::RunMode mode = server ? rtype::ecs::RunMode::kServer : rtype::ecs::RunMode::kClient;
        std::vector<std::string> extraPlugins;
        if (editor) {
            mode = rtype::ecs::RunMode::kStandalone;
            extraPlugins.emplace_back(kEditorPlugin);
        }
        rtype::engine::Engine engine{{
            .mode = mode,
            .args = std::move(args),
            .extraPlugins = std::move(extraPlugins),
        }};

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
