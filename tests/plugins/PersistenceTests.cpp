/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PersistenceTests
*/

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "plugins/persistence/PersistenceCalls.hpp"
#include "plugins/persistence/PersistencePlugin.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"
#include "plugins/persistence/SaveStore.hpp"
#include "rtype/ecs/app/App.hpp"

namespace {
using rtype::plugins::persistence::SaveStore;

/// A fresh empty directory, removed at the end of each test.
class PersistenceFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        static std::atomic<int> counter{0};
        _directory = std::filesystem::temp_directory_path() / ("rtype-persistence-test-" + std::to_string(++counter) +
                                                               "-" + std::to_string(std::random_device{}()));
        std::filesystem::remove_all(_directory);
    }
    void TearDown() override {
        std::error_code code;
        std::filesystem::remove_all(_directory, code);
    }
    [[nodiscard]] std::string slurp(const std::filesystem::path& path) const {
        std::ifstream file{path, std::ios::binary};
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
    std::filesystem::path _directory;
};
}  // namespace

TEST_F(PersistenceFixture, RoundTripsAnyBytes) {
    const SaveStore store{_directory};
    std::string data = "{\"schema_version\":3}\n";
    data += std::string("\0\n\xff", 3);
    ASSERT_TRUE(store.write("slot1", data).ok);
    const auto loaded = store.read("slot1");
    EXPECT_TRUE(loaded.ok);
    EXPECT_EQ(loaded.data, data);
    EXPECT_TRUE(loaded.error.empty());
    EXPECT_TRUE(store.exists("slot1"));
}

TEST_F(PersistenceFixture, RoundTripsAnEmptyText) {
    const SaveStore store{_directory};
    ASSERT_TRUE(store.write("empty", "").ok);
    const auto loaded = store.read("empty");
    EXPECT_TRUE(loaded.ok);
    EXPECT_TRUE(loaded.data.empty());
}

TEST_F(PersistenceFixture, OverwritesAnExistingSave) {
    const SaveStore store{_directory};
    ASSERT_TRUE(store.write("save", "one").ok);
    ASSERT_TRUE(store.write("save", "two").ok);
    EXPECT_EQ(store.read("save").data, "two");
    EXPECT_FALSE(std::filesystem::exists(_directory / "save.sav.tmp"));
}

TEST_F(PersistenceFixture, ReadingAMissingSaveFailsCleanly) {
    const SaveStore store{_directory};
    const auto loaded = store.read("nothing");
    EXPECT_FALSE(loaded.ok);
    EXPECT_EQ(loaded.error, "not_found");
    EXPECT_FALSE(store.exists("nothing"));
}

TEST_F(PersistenceFixture, ReadingATruncatedFileIsCorrupt) {
    const SaveStore store{_directory};
    ASSERT_TRUE(store.write("save", "some long enough content").ok);
    const std::filesystem::path file = _directory / "save.sav";
    const std::string whole = slurp(file);
    for (const std::size_t keep : {std::size_t{0}, std::size_t{3}, std::size_t{9}, whole.size() - 1}) {
        std::ofstream{file, std::ios::binary | std::ios::trunc}.write(whole.data(), static_cast<std::streamsize>(keep));
        const auto loaded = store.read("save");
        EXPECT_FALSE(loaded.ok) << keep;
        EXPECT_EQ(loaded.error, "corrupt") << keep;
    }
}

TEST_F(PersistenceFixture, ReadingADamagedOrForeignFileIsCorrupt) {
    const SaveStore store{_directory};
    ASSERT_TRUE(store.write("save", "abcdef").ok);
    const std::filesystem::path file = _directory / "save.sav";
    std::string damaged = slurp(file);
    damaged.back() = 'X';
    std::ofstream{file, std::ios::binary | std::ios::trunc} << damaged;
    EXPECT_EQ(store.read("save").error, "corrupt");
    std::ofstream{file, std::ios::binary | std::ios::trunc} << "not a save file at all";
    EXPECT_EQ(store.read("save").error, "corrupt");
    std::ofstream{file, std::ios::binary | std::ios::trunc} << "RTSAVE1\nzz\nzz\nabc";
    EXPECT_EQ(store.read("save").error, "corrupt");
}

TEST_F(PersistenceFixture, AnInterruptedWriteLeavesTheOldSaveIntact) {
    const SaveStore store{_directory};
    ASSERT_TRUE(store.write("save", "old").ok);
    // A crash between the temporary file and the rename leaves a partial temporary file next to the save.
    std::ofstream{_directory / "save.sav.tmp", std::ios::binary} << "RTSAVE1\n99\nbeef\npartial";
    EXPECT_EQ(store.read("save").data, "old");
    // A write that cannot even create its temporary file fails and keeps the old save.
    std::filesystem::remove(_directory / "save.sav.tmp");
    std::filesystem::create_directory(_directory / "save.sav.tmp");
    const auto failed = store.write("save", "new");
    EXPECT_FALSE(failed.ok);
    EXPECT_TRUE(failed.error.starts_with("io:"));
    EXPECT_EQ(store.read("save").data, "old");
    // The next write after a leftover temporary file succeeds.
    std::filesystem::remove(_directory / "save.sav.tmp");
    std::ofstream{_directory / "save.sav.tmp", std::ios::binary} << "garbage";
    ASSERT_TRUE(store.write("save", "new").ok);
    EXPECT_EQ(store.read("save").data, "new");
}

TEST_F(PersistenceFixture, DeleteRemovesTheSaveAndIsIdempotent) {
    const SaveStore store{_directory};
    ASSERT_TRUE(store.write("save", "x").ok);
    std::ofstream{_directory / "save.sav.tmp"} << "leftover";
    EXPECT_TRUE(store.remove("save").ok);
    EXPECT_FALSE(store.exists("save"));
    EXPECT_FALSE(std::filesystem::exists(_directory / "save.sav.tmp"));
    EXPECT_EQ(store.read("save").error, "not_found");
    EXPECT_TRUE(store.remove("save").ok);
}

TEST_F(PersistenceFixture, DeleteWorksOnACorruptSave) {
    const SaveStore store{_directory};
    std::filesystem::create_directories(_directory);
    std::ofstream{_directory / "save.sav"} << "junk";
    EXPECT_EQ(store.read("save").error, "corrupt");
    EXPECT_TRUE(store.remove("save").ok);
    EXPECT_FALSE(store.exists("save"));
}

TEST_F(PersistenceFixture, RejectsNamesThatEscapeTheDirectory) {
    const SaveStore store{_directory};
    for (const char* name : {"", "../x", "a/b", "a\\b", ".hidden", "..", "with space"}) {
        EXPECT_FALSE(SaveStore::isValidName(name)) << name;
        EXPECT_EQ(store.write(name, "x").error, "invalid_name") << name;
        EXPECT_EQ(store.read(name).error, "invalid_name") << name;
        EXPECT_EQ(store.remove(name).error, "invalid_name") << name;
        EXPECT_FALSE(store.exists(name)) << name;
    }
    EXPECT_TRUE(SaveStore::isValidName("profile_1.v2-backup"));
}

TEST(PersistenceSettings, ReadsDirectoryAndFolder) {
    using rtype::plugins::persistence::PersistenceSettings;
    const auto defaults = PersistenceSettings::fromSettings(rtype::core::config::Settings{});
    EXPECT_TRUE(defaults.directory.empty());
    EXPECT_EQ(defaults.folder, "rtype");
}

TEST_F(PersistenceFixture, PluginRegistersTheCallsOnItsDirectory) {
    using namespace rtype::plugins::persistence;
    rtype::ecs::App app;
    app.addPlugin(PersistencePlugin{PersistenceSettings{.directory = _directory.string(), .folder = "rtype"}});
    rtype::ecs::World& world = app.getWorld();
    for (const auto call : {kReadCall, kWriteCall, kDeleteCall, kExistsCall}) {
        EXPECT_NE(world.getCalls().findByName(call), nullptr) << call;
    }
    const SaveStore* store = world.getResources().get<SaveStore>();
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store->getDirectory(), _directory);
    ASSERT_TRUE(store->write("save", "hello").ok);
    EXPECT_EQ(store->read("save").data, "hello");
}
