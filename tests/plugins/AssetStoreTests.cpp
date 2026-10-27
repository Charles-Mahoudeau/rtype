/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetStoreTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "plugins/asset/AssetFiles.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"

using rtype::plugins::asset::AssetId;
using rtype::plugins::asset::AssetStore;

namespace {
/// @brief A text file in the temporary directory, removed when the test ends.
class TemporaryFile {
  public:
    TemporaryFile(const std::string& name, std::string_view text)
        : _path{std::filesystem::temp_directory_path() / name} {
        std::ofstream file{_path, std::ios::binary};
        file << text;
    }
    ~TemporaryFile() { std::filesystem::remove(_path); }
    TemporaryFile(const TemporaryFile& other) = delete;
    TemporaryFile& operator=(const TemporaryFile& other) = delete;
    TemporaryFile(TemporaryFile&& other) = delete;
    TemporaryFile& operator=(TemporaryFile&& other) = delete;

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< Where the file was written.
};

/// @brief Reads a file as its text, counting its calls; throws on the text "bad".
struct Parse {
    int* calls;  ///< Incremented on each call.

    std::string operator()(std::string_view text, const std::filesystem::path& /*path*/) const {
        ++*calls;
        if (text == "bad") {
            throw std::runtime_error{"bad text"};
        }
        return std::string{text};
    }
};
}  // namespace

TEST(AssetFiles, NormalizesAPathAsAssetIdsSeeIt) {
    EXPECT_EQ(rtype::plugins::asset::normalizePath("assets/./fonts/../fonts/a.json"), "assets/fonts/a.json");
    EXPECT_EQ(AssetId::fromPath("assets/./a.png"), AssetId::fromPath("assets/a.png"));
}

TEST(AssetFiles, ReadsAWholeFileOrThrows) {
    const TemporaryFile file{"rtype-asset-store-tests.txt", "line 1\nline 2"};
    EXPECT_EQ(rtype::plugins::asset::readTextFile(file.getPath()), "line 1\nline 2");
    EXPECT_THROW(static_cast<void>(rtype::plugins::asset::readTextFile("no/such/file.txt")),
                 rtype::plugins::backend::exceptions::AssetLoadException);
}

TEST(AssetStore, ReadsAFileOnce) {
    const TemporaryFile file{"rtype-asset-store-tests.txt", "hello"};
    AssetStore<std::string> store{"Text"};
    int calls = 0;
    const auto first = store.load(file.getPath(), Parse{&calls});
    ASSERT_NE(first.added, nullptr);
    EXPECT_EQ(*first.added, "hello");
    EXPECT_EQ(first.id, AssetId::fromPath(file.getPath()));
    const auto second = store.load(file.getPath(), Parse{&calls});
    EXPECT_EQ(second.id, first.id);
    EXPECT_EQ(second.added, nullptr);  // Known: not read again.
    EXPECT_EQ(calls, 1);
    ASSERT_NE(store.get(first.id), nullptr);
    EXPECT_EQ(*store.get(first.id), "hello");
    EXPECT_EQ(store.getCount(), 1U);
}

TEST(AssetStore, RecordsWhyAnAssetFailed) {
    const TemporaryFile file{"rtype-asset-store-tests.txt", "bad"};
    AssetStore<std::string> store{"Text"};
    int calls = 0;
    const auto parsed = store.load(file.getPath(), Parse{&calls});
    EXPECT_EQ(parsed.added, nullptr);
    EXPECT_EQ(store.get(parsed.id), nullptr);
    ASSERT_NE(store.getEntry(parsed.id), nullptr);
    EXPECT_EQ(store.getEntry(parsed.id)->error, "bad text");
    const auto missing = store.load("no/such/file.txt", Parse{&calls});
    ASSERT_NE(store.getEntry(missing.id), nullptr);
    EXPECT_EQ(store.getEntry(missing.id)->error.rfind("Text 'no/such/file.txt': ", 0), 0U);  // Named, then why.
    EXPECT_EQ(calls, 1);  // A file that cannot be opened is never parsed.
}

TEST(AssetStore, KeepsTheFirstAssetAddedUnderAName) {
    AssetStore<std::string> store{"Text"};
    const auto first = store.add("greeting", "hello");
    ASSERT_NE(first.added, nullptr);
    const auto second = store.add("./greeting", "bye");  // The same name, normalized.
    EXPECT_EQ(second.id, first.id);
    EXPECT_EQ(second.added, nullptr);
    EXPECT_EQ(*store.get(first.id), "hello");
}

TEST(AssetStore, ForgetsAnUnloadedAsset) {
    AssetStore<std::string> store{"Text"};
    const AssetId id = store.add("greeting", "hello").id;
    EXPECT_TRUE(store.unload(id));
    EXPECT_EQ(store.getEntry(id), nullptr);
    EXPECT_FALSE(store.unload(id));
    EXPECT_NE(store.add("greeting", "again").added, nullptr);
}
