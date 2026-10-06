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
#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/event/Event.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"

using namespace rtype::engine::event;
using example::SfmlPlatform;
using example::SfmlRenderer;
using rtype::engine::backend::Backend;
using rtype::engine::backend::BackendRegistry;
using rtype::engine::config::Settings;
using rtype::engine::graphics::Color;
using rtype::engine::graphics::IRenderer;
using rtype::engine::input::Key;
using rtype::engine::platform::IPlatform;

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
