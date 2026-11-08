/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-editor: the editor of a game, in `game/` (or the <directory> given first). It runs both sides of the game in one
// world (no network) inside the editor's panels, always as a debug run (`engine.isDebug`). The game is shipped with
// aylo-runtime. Every flag goes to project.luau as `engine.args`, and `engine.args.editor` is always there.
//   aylo-editor [<directory>] [--<flag> <value>]...
//   aylo-editor --version

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/version/Version.hpp"
#include "editor/EditorPlugin.hpp"
#include "editor/EditorSettings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"

namespace {
constexpr int kErrorCode = 84;                          ///< What the program returns when it cannot run the game.
constexpr std::string_view kServerFlag = "--server";    ///< Refused: the editor runs both sides.
constexpr std::string_view kVersionFlag = "--version";  ///< Prints the version of the engine and stops.
constexpr std::string_view kEditorFlag = "--editor";    ///< Given to the project, so it can tell it runs in the editor.

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
        if (std::ranges::find(args, kServerFlag) != args.end()) {
            std::cerr << "Error: --server is not for the editor: it runs both sides\n";
            return kErrorCode;
        }
        if (std::ranges::find(args, kEditorFlag) == args.end()) {
            args.emplace_back(kEditorFlag);
        }
        rtype::engine::Engine engine{{
            .mode = rtype::ecs::RunMode::kStandalone,
            .args = std::move(args),
            .addPlugins =
                [](rtype::ecs::App& app, const rtype::core::config::Settings& settings) {
                    app.addPlugin(rtype::editor::EditorPlugin{rtype::editor::readEditorOptions(settings)});
                },
            .debug = true,
        }};

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
