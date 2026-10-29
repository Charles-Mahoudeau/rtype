/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEffectAssetsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/particle/IParticleEffectLoader.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::backend::exceptions::ParticleException;
using rtype::plugins::backend::graphics::BlendMode;
using rtype::plugins::particle::IParticleEffectLoader;
using rtype::plugins::particle::ParticleEffect;
using rtype::plugins::particle::ParticleEffectAssets;

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

/// @brief A loader whose files hold one number, the effect's capacity, counting the files it reads.
class CapacityLoader : public IParticleEffectLoader {
  public:
    explicit CapacityLoader(int& calls) : _calls{&calls} {}
    ~CapacityLoader() override = default;
    CapacityLoader(const CapacityLoader& other) = delete;
    CapacityLoader& operator=(const CapacityLoader& other) = delete;
    CapacityLoader(CapacityLoader&& other) = delete;
    CapacityLoader& operator=(CapacityLoader&& other) = delete;

    [[nodiscard]] ParticleEffect load(std::string_view text, const std::filesystem::path& path) const override {
        ++*_calls;
        if (text == "broken") {
            throw std::runtime_error{std::format("'{}' is broken", path.string())};
        }
        ParticleEffect effect;
        effect.desc.capacity = static_cast<std::uint32_t>(std::stoul(std::string{text}));
        return effect;
    }

  private:
    int* _calls;  ///< Files read, owned by the test.
};

/// @return Assets with a CapacityLoader registered for ".fx", counting into @p calls.
ParticleEffectAssets makeAssets(int& calls) {
    ParticleEffectAssets assets;
    assets.addLoader(".fx", std::make_unique<CapacityLoader>(calls));
    return assets;
}
}  // namespace

static_assert(rtype::ecs::Resource<ParticleEffectAssets>);

TEST(ParticleEffectAssetsTests, DefaultEffectLooksAdditiveAndIsValid) {
    ParticleEffectAssets assets;
    const ParticleEffect effect;
    EXPECT_EQ(effect.blend, BlendMode::kAdditive);
    EXPECT_EQ(effect.texture, AssetId{});
    EXPECT_NO_THROW(static_cast<void>(assets.add("default", effect)));
}

TEST(ParticleEffectAssetsTests, AddStoresAnEffectByName) {
    ParticleEffectAssets assets;
    ParticleEffect effect;
    effect.desc.capacity = 64;
    const AssetId id = assets.add("sparks", std::move(effect));

    EXPECT_EQ(id, AssetId::fromPath("sparks"));
    EXPECT_EQ(assets.getCount(), 1U);
    ASSERT_NE(assets.get(id), nullptr);
    EXPECT_EQ(assets.get(id)->desc.capacity, 64U);
    ASSERT_NE(assets.getEntry(id), nullptr);
    EXPECT_EQ(assets.getEntry(id)->path, "sparks");
}

TEST(ParticleEffectAssetsTests, AddingANameTwiceKeepsTheFirstEffect) {
    ParticleEffectAssets assets;
    ParticleEffect first;
    first.desc.capacity = 8;
    ParticleEffect second;
    second.desc.capacity = 16;
    const AssetId id = assets.add("smoke", std::move(first));

    EXPECT_EQ(assets.add("smoke", std::move(second)), id);
    EXPECT_EQ(assets.getCount(), 1U);
    EXPECT_EQ(assets.get(id)->desc.capacity, 8U);
}

TEST(ParticleEffectAssetsTests, AddThrowsOnAnInvalidEffect) {
    ParticleEffectAssets assets;
    ParticleEffect effect;
    effect.desc.capacity = 0;

    EXPECT_THROW(static_cast<void>(assets.add("empty", std::move(effect))), ParticleException);
    EXPECT_EQ(assets.getCount(), 0U);
}

TEST(ParticleEffectAssetsTests, UnknownIdGivesNothing) {
    const ParticleEffectAssets assets;
    EXPECT_EQ(assets.get(AssetId::fromPath("missing")), nullptr);
    EXPECT_EQ(assets.getEntry(AssetId::fromPath("missing")), nullptr);
}

TEST(ParticleEffectAssetsTests, UnloadForgetsAnEffect) {
    ParticleEffectAssets assets;
    const AssetId id = assets.add("sparks", ParticleEffect{});

    EXPECT_TRUE(assets.unload(id));
    EXPECT_EQ(assets.get(id), nullptr);
    EXPECT_EQ(assets.getCount(), 0U);
    EXPECT_FALSE(assets.unload(id));
}

TEST(ParticleEffectAssetsTests, NoLoaderIsRegisteredByDefault) {
    const ParticleEffectAssets assets;
    EXPECT_TRUE(assets.getLoaderExtensions().empty());
}

TEST(ParticleEffectAssetsTests, AddLoaderListsItsExtensionsSorted) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    assets.addLoader(".luau", std::make_unique<CapacityLoader>(calls));
    assets.addLoader(".fx", std::make_unique<CapacityLoader>(calls));

    EXPECT_EQ(assets.getLoaderExtensions(), (std::vector<std::string>{".fx", ".luau"}));
}

TEST(ParticleEffectAssetsTests, AddLoaderRejectsAnExtensionWithoutDotOrANullLoader) {
    int calls = 0;
    ParticleEffectAssets assets;
    EXPECT_THROW(assets.addLoader("luau", std::make_unique<CapacityLoader>(calls)), std::invalid_argument);
    EXPECT_THROW(assets.addLoader(".", std::make_unique<CapacityLoader>(calls)), std::invalid_argument);
    EXPECT_THROW(assets.addLoader(".luau", nullptr), std::invalid_argument);
    EXPECT_TRUE(assets.getLoaderExtensions().empty());
}

TEST(ParticleEffectAssetsTests, LoadReadsAFileWithTheLoaderOfItsExtension) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    const TemporaryFile file{"rtype_particle_effect_load.fx", "256"};
    const AssetId id = assets.load(file.getPath());

    EXPECT_EQ(id, AssetId::fromPath(file.getPath()));
    EXPECT_EQ(calls, 1);
    ASSERT_NE(assets.get(id), nullptr);
    EXPECT_EQ(assets.get(id)->desc.capacity, 256U);
}

TEST(ParticleEffectAssetsTests, LoadingAFileTwiceReadsItOnce) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    const TemporaryFile file{"rtype_particle_effect_twice.fx", "32"};
    const AssetId id = assets.load(file.getPath());

    EXPECT_EQ(assets.load(file.getPath()), id);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(assets.getCount(), 1U);
}

TEST(ParticleEffectAssetsTests, LoadRecordsAFileWithoutLoaderAsFailed) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    const TemporaryFile file{"rtype_particle_effect_unknown.txt", "32"};
    const AssetId id = assets.load(file.getPath());

    EXPECT_EQ(calls, 0);
    EXPECT_EQ(assets.get(id), nullptr);
    ASSERT_NE(assets.getEntry(id), nullptr);
    EXPECT_NE(assets.getEntry(id)->error.find("no loader for the extension '.txt'"), std::string::npos);
}

TEST(ParticleEffectAssetsTests, LoadRecordsAMissingFileAsFailed) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    const AssetId id = assets.load(std::filesystem::temp_directory_path() / "rtype_particle_effect_missing.fx");

    EXPECT_EQ(calls, 0);
    EXPECT_EQ(assets.get(id), nullptr);
    ASSERT_NE(assets.getEntry(id), nullptr);
    EXPECT_FALSE(assets.getEntry(id)->error.empty());
}

TEST(ParticleEffectAssetsTests, LoadRecordsALoaderErrorAsFailed) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    const TemporaryFile file{"rtype_particle_effect_broken.fx", "broken"};
    const AssetId id = assets.load(file.getPath());

    EXPECT_EQ(assets.get(id), nullptr);
    ASSERT_NE(assets.getEntry(id), nullptr);
    EXPECT_NE(assets.getEntry(id)->error.find("is broken"), std::string::npos);
}

TEST(ParticleEffectAssetsTests, LoadRecordsAnInvalidEffectAsFailed) {
    int calls = 0;
    ParticleEffectAssets assets = makeAssets(calls);
    const TemporaryFile file{"rtype_particle_effect_invalid.fx", "0"};
    const AssetId id = assets.load(file.getPath());

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(assets.get(id), nullptr);
    ASSERT_NE(assets.getEntry(id), nullptr);
    EXPECT_NE(assets.getEntry(id)->error.find("rtype_particle_effect_invalid.fx"), std::string::npos);
    EXPECT_NE(assets.getEntry(id)->error.find("capacity"), std::string::npos);
}

TEST(ParticleEffectAssetsTests, AddLoaderReplacesThePreviousLoaderOfAnExtension) {
    int firstCalls = 0;
    int secondCalls = 0;
    ParticleEffectAssets assets = makeAssets(firstCalls);
    assets.addLoader(".fx", std::make_unique<CapacityLoader>(secondCalls));
    const TemporaryFile file{"rtype_particle_effect_replaced.fx", "16"};
    static_cast<void>(assets.load(file.getPath()));

    EXPECT_EQ(firstCalls, 0);
    EXPECT_EQ(secondCalls, 1);
}
