/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LoaderTests
*/

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "render/vulkan/core/Loader.hpp"

namespace {

using rtype::render::vulkan::core::Loader;

// The machine running the tests may or may not have a Vulkan loader (a CI container usually has none): both outcomes
// are right, as long as each is the documented one.

TEST(Loader, opensTheSystemLoaderOrNamesWhatIsMissing) {
    try {
        const Loader loader;
        EXPECT_NE(loader.getEntryPoint(), nullptr);
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string{error.what()}.find("Vulkan loader not found"), std::string::npos) << error.what();
    }
}

TEST(Loader, opensTheSameLoaderEachTime) {
    try {
        const Loader first;
        const Loader second;
        EXPECT_EQ(first.getEntryPoint(), second.getEntryPoint());
    } catch (const std::runtime_error&) {
        GTEST_SKIP() << "No Vulkan loader on this machine";
    }
}

}  // namespace
