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
#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"

using namespace rtype::contract::platform::event;
using example::SfmlPlatform;
using example::SfmlRenderer;
using rtype::contract::platform::IPlatform;
using rtype::contract::platform::input::Key;
using rtype::contract::renderer::Color;
using rtype::contract::renderer::IRenderer;
using rtype::core::config::Settings;
using rtype::plugins::graphics::Backend;
using rtype::plugins::graphics::BackendRegistry;

int main() {
    try {
        /// @note Your own pair, platform and renderer, both written in this example: "sfml" and "sfml". A
        /// platform and a renderer may share a name: they live in two separate lists. Neither has settings.
        BackendRegistry registry;
        registry.platforms.add("sfml", [](const Settings& settings) -> std::unique_ptr<IPlatform> {
            settings.checkKeys({}, "sfml");
            return std::make_unique<SfmlPlatform>();
        });
        registry.renderers.add("sfml", [](const Settings& settings) -> std::unique_ptr<IRenderer> {
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
