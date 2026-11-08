/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NullBackendTests
*/

#include <gtest/gtest.h>

#include <variant>

#include "backend/null/NullBackend.hpp"

namespace rtype::test::plugins {

TEST(NullBackendTests, PlatformReportsConfiguredSizeAndNoEvents) {
    backend::null::NullPlatform platform;
    contract::platform::WindowConfig config;
    config.size = {640, 480};
    config.title = "headless";
    platform.init(config);
    EXPECT_EQ(platform.getFramebufferSize(), glm::uvec2(640, 480));
    EXPECT_EQ(platform.getTitle(), "headless");
    EXPECT_TRUE(platform.isOpen());
    EXPECT_TRUE(platform.pollEvents().empty());
}

TEST(NullBackendTests, CloseSendsOneClosedEvent) {
    backend::null::NullPlatform platform;
    platform.init({});
    platform.close();
    EXPECT_FALSE(platform.isOpen());
    const auto events = platform.pollEvents();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_TRUE(std::holds_alternative<contract::platform::event::Closed>(events.front()));
    EXPECT_TRUE(platform.pollEvents().empty());
}

TEST(NullBackendTests, RendererGivesDistinctValidIds) {
    backend::null::NullRenderer renderer;
    const auto first = renderer.createTexture({}, {});
    const auto second = renderer.createTexture({}, {});
    EXPECT_NE(first.generation, 0U);
    EXPECT_FALSE(first == second);
    renderer.beginFrame(contract::renderer::Color{0.0F});
    renderer.endFrame();
}

}  // namespace rtype::test::plugins
