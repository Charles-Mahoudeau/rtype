/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FontAssetsTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::text::Font;
using rtype::plugins::text::FontAssets;

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

/// @brief A font of one glyph, as msdf-atlas-gen writes it.
constexpr std::string_view kFontJson = R"({
  "atlas": { "type": "msdf", "distanceRange": 4, "size": 48, "width": 64, "height": 64, "yOrigin": "top" },
  "metrics": { "emSize": 1, "lineHeight": 1.25, "ascender": -1, "descender": 0.25 },
  "glyphs": [ { "unicode": 32, "advance": 0.25 } ]
})";

/// @return A font built in code, its atlas @p image.
Font makeFont(const std::string& image) { return Font{image, {}, 4.0F, Font::Metrics{}}; }
}  // namespace

static_assert(rtype::ecs::Resource<FontAssets>);

TEST(FontAssets, ReadsAFontFileAtOnce) {
    const TemporaryFile json{"rtype-font-tests.json", kFontJson};
    FontAssets fonts;
    const AssetId id = fonts.load(json.getPath());
    EXPECT_EQ(id, AssetId::fromPath(json.getPath().lexically_normal().generic_string()));
    const Font* font = fonts.get(id);
    ASSERT_NE(font, nullptr);
    EXPECT_EQ(font->getGlyphCount(), 1U);
    EXPECT_EQ(fonts.load(json.getPath()), id);  // Asked twice: read once.
    EXPECT_EQ(fonts.getCount(), 1U);
}

TEST(FontAssets, AsksForTheAtlasOfEachFontOnce) {
    const TemporaryFile json{"rtype-font-tests.json", kFontJson};
    FontAssets fonts;
    static_cast<void>(fonts.load(json.getPath()));
    static_cast<void>(fonts.load(json.getPath()));
    const std::vector<FontAssets::TextureRequest> requests = fonts.takeTextureRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(std::filesystem::path{requests.at(0).path}.filename(), "rtype-font-tests.png");
    EXPECT_EQ(requests.at(0).desc.encoding, rtype::plugins::backend::graphics::TextureEncoding::kLinear);
    EXPECT_EQ(requests.at(0).desc.filter, rtype::plugins::backend::graphics::TextureFilter::kLinear);
    EXPECT_TRUE(fonts.takeTextureRequests().empty());
}

TEST(FontAssets, RecordsAFontThatCannotBeRead) {
    FontAssets fonts;
    const AssetId id = fonts.load("no/such/font.json");
    EXPECT_EQ(fonts.get(id), nullptr);
    const FontAssets::Entry* entry = fonts.getEntry(id);
    ASSERT_NE(entry, nullptr);
    EXPECT_NE(entry->error.find("no/such/font.json"), std::string::npos);
    EXPECT_TRUE(fonts.takeTextureRequests().empty());
}

TEST(FontAssets, AddsAndUnloadsAFontBuiltInCode) {
    FontAssets fonts;
    const AssetId id = fonts.add("ui", makeFont("ui.png"));
    EXPECT_EQ(id, AssetId::fromPath("ui"));
    ASSERT_NE(fonts.get(id), nullptr);
    EXPECT_EQ(fonts.get(id)->getTexturePath(), "ui.png");
    EXPECT_EQ(fonts.add("ui", makeFont("other.png")), id);  // The first font is kept.
    EXPECT_EQ(fonts.get(id)->getTexturePath(), "ui.png");
    EXPECT_TRUE(fonts.unload(id));
    EXPECT_EQ(fonts.get(id), nullptr);
    EXPECT_FALSE(fonts.unload(id));
}
