/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextPluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::backend::math::Rect;
using rtype::plugins::text::Font;
using rtype::plugins::text::FontAssets;
using rtype::plugins::text::Glyph;
using rtype::plugins::text::TextRenderer;
using rtype::plugins::transform::Transform;

namespace {
/// @brief An App with the BackendPlugin over the fake backend, and the font "ui": an 'A' one em high and half an em
/// wide, the baseline one em down, lines 1.5 ems apart, its atlas font.png (2x1 pixels) loaded.
class TextApp {
  public:
    TextApp() : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{rtype::ecs::RunMode::kClient} {
        _app.addPlugin(rtype::plugins::backend::BackendPlugin{rtype::test::plugins::makeFakeRegistry(_state),
                                                              rtype::test::plugins::makeFakeSettings()});
        _app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}));
        static_cast<void>(world().getResources().getMut<rtype::plugins::backend::TextureAssets>()->load(
            "font.png", rtype::test::plugins::twoPixelPng()));
        Font font{"font.png", {}, 3.0F, Font::Metrics{.lineHeight = 1.5F, .ascender = -1.0F, .descender = 0.5F}};
        font.addGlyph(U'A', Glyph{.advance = 0.5F,
                                  .plane = Rect{{0.0F, -1.0F}, {0.5F, 1.0F}},
                                  .atlas = Rect{{0.0F, 0.0F}, {1.0F, 1.0F}}});
        _font = world().getResources().getMut<FontAssets>()->add("ui", font);
    }
    ~TextApp() = default;
    TextApp(const TextApp& other) = delete;
    TextApp& operator=(const TextApp& other) = delete;
    TextApp(TextApp&& other) = delete;
    TextApp& operator=(TextApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    [[nodiscard]] AssetId font() const noexcept { return _font; }

    /// @brief Spawns an entity at @p position drawing @p text.
    rtype::ecs::Entity spawnText(glm::vec2 position, TextRenderer text) {
        const rtype::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, Transform{.position = position});
        world().getComponents().add(entity, std::move(text));
        return entity;
    }

    /// @return The texts drawn by one more frame.
    std::vector<rtype::plugins::backend::graphics::Text> drawFrame() {
        _state->texts.clear();
        _state->drawOrder.clear();
        _app.runFrame();
        return _state->texts;
    }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
    AssetId _font;                                                   ///< The font "ui".
};
}  // namespace

TEST(TextPlugin, ComesWithTheBackendPlugin) {
    TextApp app;
    EXPECT_TRUE(app.app().hasPlugin(rtype::plugins::text::TextPlugin::kName));
    EXPECT_TRUE(app.world().getResources().has<FontAssets>());
    EXPECT_TRUE(app.world().getComponents().find("rtype.TextRenderer"));
}

TEST(TextPlugin, DrawsATextAtItsEntity) {
    TextApp app;
    static_cast<void>(app.spawnText(
        {100.0F, 50.0F},
        TextRenderer{.text = "AA", .font = app.font(), .size = 10.0F, .color = {1.0F, 0.0F, 0.0F, 1.0F}}));
    const auto texts = app.drawFrame();
    ASSERT_EQ(texts.size(), 1U);
    const rtype::plugins::backend::graphics::Text& text = texts.at(0);
    EXPECT_EQ(text.position, (glm::vec2{100.0F, 50.0F}));
    EXPECT_EQ(text.glyphs.size(), 2U);
    EXPECT_FLOAT_EQ(text.glyphs.at(1).bounds.getX(), 5.0F);
    EXPECT_FLOAT_EQ(text.distanceRange, 3.0F);
    EXPECT_EQ(text.color, (glm::vec4{1.0F, 0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(text.origin, (glm::vec2{0.0F}));
}

TEST(TextPlugin, PivotsATextOnItsAnchor) {
    TextApp app;
    static_cast<void>(app.spawnText(
        {0.0F, 0.0F}, TextRenderer{.text = "AA", .font = app.font(), .size = 10.0F, .anchor = {0.5F, 1.0F}}));
    const auto texts = app.drawFrame();
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts.at(0).origin, (glm::vec2{5.0F, 15.0F}));  // Half of 10 wide, all of one 15-high line.
}

TEST(TextPlugin, SkipsATextWithoutItsFont) {
    TextApp app;
    static_cast<void>(app.spawnText({0.0F, 0.0F}, TextRenderer{.text = "AA", .font = AssetId::fromPath("missing")}));
    static_cast<void>(app.spawnText({0.0F, 0.0F}, TextRenderer{.font = app.font()}));  // And an empty one.
    EXPECT_TRUE(app.drawFrame().empty());
}

TEST(TextPlugin, SortsTheTextsWithTheSpritesByLayer) {
    TextApp app;
    static_cast<void>(app.spawnText({0.0F, 0.0F}, TextRenderer{.text = "A", .font = app.font(), .layer = 10}));
    const rtype::ecs::Entity ship = app.world().spawn();
    app.world().getComponents().add(ship, Transform{});
    app.world().getComponents().add(
        ship, rtype::plugins::backend::SpriteRenderer{.texture = AssetId::fromPath("font.png"), .layer = 5});
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'s', 't'}));  // The text, layer 10, on top.
}
