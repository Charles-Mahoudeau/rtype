/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginLoaderTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <system_error>

#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/plugin/PluginLoader.hpp"
#include "rtype/engine/plugin/SharedLibrary.hpp"

using rtype::engine::exceptions::RuntimeError;
using rtype::engine::plugin::PluginLoader;
using rtype::engine::plugin::SharedLibrary;

namespace {
/// @return The file name of the fixture library.
std::string fixtureName() {
#ifdef _WIN32
    return std::format("rtype-test-plugin{}", SharedLibrary::getExtension());
#else
    return std::format("librtype-test-plugin{}", SharedLibrary::getExtension());
#endif
}

/// @brief Two plugin folders of their own, removed when the test ends.
class PluginLoaderTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_first);
        std::filesystem::create_directories(_second);
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    /// @brief Copies the fixture library to @p directory.
    static void addFixture(const std::filesystem::path& directory) {
        std::filesystem::copy_file(rtype::core::system::getExecutableDirectory() / "plugins" / fixtureName(),
                                   directory / fixtureName());
    }

    std::filesystem::path _root{std::filesystem::temp_directory_path() /
                                std::format("rtype_plugin_loader_{:08x}", std::random_device{}())};
    std::filesystem::path _first{_root / "first"};
    std::filesystem::path _second{_root / "second"};
};
}  // namespace

TEST_F(PluginLoaderTest, LoadsALibraryAndRegistersItsPlugins) {
    addFixture(_first);
    PluginLoader loader;
    loader.load(_first);
    EXPECT_EQ(loader.getCount(), 1U);

    rtype::ecs::App app;
    rtype::core::library::LibraryRegistrar registrar;
    registrar.provide(rtype::ecs::kPluginRegistryService, app.getPluginRegistry());
    loader.registerInto(registrar);
    ASSERT_TRUE(app.getPluginRegistry().has("test.Loaded"));
    app.addPlugin(app.getPluginRegistry().create("test.Loaded"));
    EXPECT_TRUE(app.hasPlugin("test.Loaded"));
}

TEST_F(PluginLoaderTest, KeepsTheCodeOfALibraryOnceItsLoaderIsGone) {
    addFixture(_first);
    rtype::ecs::App app;
    {
        PluginLoader loader;
        loader.load(_first);
        rtype::core::library::LibraryRegistrar registrar;
        registrar.provide(rtype::ecs::kPluginRegistryService, app.getPluginRegistry());
        loader.registerInto(registrar);
    }
    // The factory is code of the library: an unloaded library would crash here, as an exception it threw would in main.
    ASSERT_TRUE(app.getPluginRegistry().has("test.Loaded"));
    app.addPlugin(app.getPluginRegistry().create("test.Loaded"));
    EXPECT_TRUE(app.hasPlugin("test.Loaded"));
}

TEST_F(PluginLoaderTest, ReadsAFolderOnce) {
    addFixture(_first);
    PluginLoader loader;
    loader.load(_first);
    loader.load(_first);
    EXPECT_EQ(loader.getCount(), 1U);
}

TEST_F(PluginLoaderTest, AMissingFolderHasNoLibrary) {
    PluginLoader loader;
    loader.load(_root / "missing");
    EXPECT_EQ(loader.getCount(), 0U);
}

TEST_F(PluginLoaderTest, IgnoresWhatIsNotALibrary) {
    std::ofstream{_first / "readme.txt"} << "text";
    PluginLoader loader;
    loader.load(_first);
    EXPECT_EQ(loader.getCount(), 0U);
}

TEST_F(PluginLoaderTest, RefusesTheSameNameInTwoFolders) {
    addFixture(_first);
    addFixture(_second);
    PluginLoader loader;
    loader.load(_first);
    try {
        loader.load(_second);
        FAIL() << "the second library was accepted";
    } catch (const RuntimeError& error) {
        EXPECT_TRUE(std::string{error.what()}.contains("same name"));
    }
}

TEST_F(PluginLoaderTest, RefusesAFileThatIsNotALibraryAndNamesIt) {
    const std::string name = std::format("broken{}", SharedLibrary::getExtension());
    std::ofstream{_first / name} << "not a library";
    PluginLoader loader;
    try {
        loader.load(_first);
        FAIL() << "the file was accepted";
    } catch (const RuntimeError& error) {
        EXPECT_TRUE(std::string{error.what()}.contains(name));
    }
}
