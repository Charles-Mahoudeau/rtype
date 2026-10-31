/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IParticleEffectLoader
*/

#pragma once

#include <filesystem>
#include <string_view>

#include "plugins/particle/ParticleEffect.hpp"

namespace rtype::plugins::particle {

/// @brief Builds an effect from a file of one format (Luau later), registered in ParticleEffectAssets for its
/// extension.
///
/// @details Lets a module that `plugins` must not depend on (the Luau one) add a file format: the executable, or code
/// that knows both, registers the loader, and the plugin needs no change to accept it.
class IParticleEffectLoader {
  public:
    IParticleEffectLoader() = default;
    virtual ~IParticleEffectLoader() = default;
    IParticleEffectLoader(const IParticleEffectLoader& other) = delete;
    IParticleEffectLoader& operator=(const IParticleEffectLoader& other) = delete;
    IParticleEffectLoader(IParticleEffectLoader&& other) = delete;
    IParticleEffectLoader& operator=(IParticleEffectLoader&& other) = delete;

    /// @brief Builds the effect the file at @p path describes. ParticleEffectAssets validates it afterwards.
    /// @param text The whole content of the file.
    /// @param path The file, for messages and for the files it refers to.
    /// @throws std::exception (any) If the file does not describe an effect, with a message that names it.
    [[nodiscard]] virtual ParticleEffect load(std::string_view text, const std::filesystem::path& path) const = 0;
};

}  // namespace rtype::plugins::particle
