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

#include "SfmlPlatform.hpp"
#include "SfmlRenderer.hpp"
#include "engine/event/Event.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"

using namespace rtype::engine::event;
using example::SfmlPlatform;
using example::SfmlRenderer;
using rtype::engine::graphics::Color;
using rtype::engine::graphics::IRenderer;
using rtype::engine::input::Key;
using rtype::engine::platform::IPlatform;

int main() {
    try {
        /// @note A full SFML pair: SfmlPlatform owns the sf::RenderWindow, SfmlRenderer draws into it. From here
        /// on, the code only sees IPlatform and IRenderer.
        const std::unique_ptr<IPlatform> platform = std::make_unique<SfmlPlatform>();
        const std::unique_ptr<IRenderer> renderer = std::make_unique<SfmlRenderer>();

        /// @note The engine's start-up sequence, unchanged. initLoader() does nothing here: neither class overrides
        /// it. renderer->init() finds the window through a dynamic_cast to SfmlPlatform.
        platform->initLoader(renderer->getLoaderEntryPoint());
        platform->init({.size = {800, 600}, .title = "SFML Window Example"});
        renderer->init(*platform);

        while (platform->isOpen()) {
            for (const auto& event : platform->pollEvents()) {
                if (const auto* resized = std::get_if<Resized>(&event); resized != nullptr) {
                    renderer->resize({resized->width, resized->height});
                }
                if (const auto* key = std::get_if<KeyPressed>(&event); key != nullptr && key->key == Key::kEscape) {
                    platform->close();
                }
            }
            renderer->beginFrame(Color{0.1F, 0.1F, 0.15F, 1.0F});
            renderer->endFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
