/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <memory>
#include <variant>
#include <vector>

#include "SfmlPlatform.hpp"
#include "SfmlRenderer.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/backend/platform/IPlatform.hpp"

using namespace rtype::plugins::backend::event;
using example::SfmlPlatform;
using example::SfmlRenderer;
using rtype::plugins::backend::Backend;
using rtype::plugins::backend::BackendRegistry;
using rtype::plugins::backend::config::Settings;
using rtype::plugins::backend::graphics::Color;
using rtype::plugins::backend::graphics::IRenderer;
using rtype::plugins::backend::input::Key;
using rtype::plugins::backend::platform::IPlatform;

int main() {
    try {
        /// @note Your own pair, platform and renderer, both written in this example: "sfml" and "sfml". A
        /// platform and a renderer may share a name: they live in two separate lists. Neither has settings.
        BackendRegistry registry;
        registry.addPlatform("sfml", [](const Settings& settings) -> std::unique_ptr<IPlatform> {
            settings.checkKeys({}, "sfml");
            return std::make_unique<SfmlPlatform>();
        });
        registry.addRenderer("sfml", [](const Settings& settings) -> std::unique_ptr<IRenderer> {
            settings.checkKeys({}, "sfml");
            return std::make_unique<SfmlRenderer>();
        });

        Settings config;
        config.set("platform", "sfml");
        config.set("renderer", "sfml");
        config.set("window.title", "SFML Window Example");
        config.set("window.size", std::vector<double>{800, 600});

        /// @note createBackend() runs the same sequence as with GLFW + Vulkan. SfmlRenderer does not override
        /// setup(): no interop is involved. Its init() finds the window through a dynamic_cast to SfmlPlatform, so
        /// pairing it with another platform throws.
        const Backend backend = registry.createBackend(config);

        while (backend.platform->isOpen()) {
            for (const auto& event : backend.platform->pollEvents()) {
                if (const auto* resized = std::get_if<Resized>(&event); resized != nullptr) {
                    backend.renderer->resize({resized->width, resized->height});
                }
                if (const auto* key = std::get_if<KeyPressed>(&event); key != nullptr && key->key == Key::kEscape) {
                    backend.platform->close();
                }
            }
            backend.renderer->beginFrame(Color{0.1F, 0.1F, 0.15F, 1.0F});
            backend.renderer->endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
