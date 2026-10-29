/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEffectAssets
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/asset/AssetStore.hpp"
#include "plugins/particle/IParticleEffectLoader.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::particle {

/// @brief The particle effects, as the ECS resource "rtype.ParticleEffectAssets" the ParticlePlugin inserts: effects
/// built in C++ are added under a name, effect files are read by the loader registered for their extension, and
/// ParticleEmitter components keep the id either gives.
///
/// @details Every effect is checked (backend::graphics::validate()) before it is stored. A file that cannot be read,
/// has no loader or describes an invalid effect is recorded as failed, with the reason, which is also printed: one
/// broken effect does not stop the game. No loader is registered by default; until the Luau one exists, effects are
/// built in C++.
/// @code
/// rtype::plugins::particle::ParticleEffect sparks;
/// sparks.desc.bursts.push_back({.count = 32});
/// sparks.desc.speed = {.min = 80.0F, .max = 160.0F};
/// sparks.texture = textures->load("assets/particles/spark.png");
/// const rtype::core::asset::AssetId id = effects->add("sparks", std::move(sparks));
/// @endcode
class ParticleEffectAssets {
  public:
    /// @brief One effect: its file (or the name it was added under), the effect (nothing if it failed) and why.
    using Entry = core::asset::AssetStore<ParticleEffect>::Entry;

    ParticleEffectAssets() = default;
    ~ParticleEffectAssets() = default;
    ParticleEffectAssets(const ParticleEffectAssets& other) = delete;
    ParticleEffectAssets& operator=(const ParticleEffectAssets& other) = delete;
    ParticleEffectAssets(ParticleEffectAssets&& other) noexcept = default;
    ParticleEffectAssets& operator=(ParticleEffectAssets&& other) noexcept = default;

    /// @return The number of effects known, read or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Registers @p loader for the files whose extension is @p extension, replacing any previous one.
    /// @param extension With its dot, as std::filesystem::path::extension() gives it: ".luau".
    /// @throws std::invalid_argument If @p extension does not start with a dot, or @p loader is null.
    void addLoader(std::string extension, std::unique_ptr<IParticleEffectLoader> loader);

    /// @return The extensions a loader is registered for, sorted.
    [[nodiscard]] std::vector<std::string> getLoaderExtensions() const;

    /// @brief Reads the file at @p path with the loader of its extension, then validates the effect.
    /// @return Its id (AssetId::fromPath()), at once, even if it failed. Asking twice gives the first one.
    /// @throws core::asset::exceptions::AssetLoadException If another path already has this id (a hash collision).
    core::asset::AssetId load(const std::filesystem::path& path);

    /// @brief Adds an effect built in code, known by @p name.
    /// @return Its id (AssetId::fromPath(name)). Adding a name twice keeps the first effect.
    /// @throws backend::exceptions::ParticleException If @p effect is invalid: a mistake in the code that built it.
    /// @throws core::asset::exceptions::AssetLoadException If another name already has this id (a hash collision).
    core::asset::AssetId add(const std::string& name, ParticleEffect effect);

    /// @return The effect @p id names, or nullptr if it is unknown or failed.
    [[nodiscard]] const ParticleEffect* get(core::asset::AssetId id) const;

    /// @return The entry of @p id, read or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(core::asset::AssetId id) const;

    /// @brief Forgets the effect @p id names. Emitters already playing it keep their particles.
    /// @return False, doing nothing, if no effect has this id.
    bool unload(core::asset::AssetId id);

  private:
    core::asset::AssetStore<ParticleEffect> _effects{"ParticleEffect"};  ///< Every effect asked for, by id.
    std::map<std::string, std::unique_ptr<IParticleEffectLoader>, std::less<>> _loaders;  ///< By extension.
};
RTYPE_ECS_RESOURCE(ParticleEffectAssets, "rtype.ParticleEffectAssets");

}  // namespace rtype::plugins::particle
