/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// r-type_client: runs the client side of the game in `game/` (or `--game <directory>`), with the GLFW platform, the
// Vulkan renderer and miniaudio. Every other flag goes to the game's project.luau as `engine.args`.
//   r-type_client [--game <directory>] [--<flag> <value>]...

#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "rtype/audio/miniaudio/Registration.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/platform/glfw/Registration.hpp"
#include "rtype/plugins/audio/AudioRegistry.hpp"
#include "rtype/plugins/backend/BackendRegistry.hpp"
#include "rtype/render/vulkan/Registration.hpp"

namespace {
constexpr int kErrorCode = 84;  ///< What the program returns when it cannot run the game.
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span args{argv, static_cast<std::size_t>(argc)};
        rtype::engine::Engine engine{{
            .mode = rtype::ecs::RunMode::kClient,
            .args = std::vector<std::string>{args.begin() + 1, args.end()},
            .registerBackends =
                [](rtype::plugins::backend::BackendRegistry& registry) {
                    rtype::platform::glfw::registerPlatform(registry);
                    rtype::render::vulkan::registerRenderer(registry);
                },
            .registerAudio =
                [](rtype::plugins::audio::AudioRegistry& registry) {
                    rtype::audio::miniaudio::registerAudio(registry);
                },
        }};

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
