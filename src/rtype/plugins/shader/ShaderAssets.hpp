/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderAssets
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/asset/AssetStore.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::shader {

/// @brief The game's shaders, compiled, as the ECS resource "rtype.ShaderAssets" the ShaderPlugin inserts: systems ask
/// for a shader and get its AssetId at once, materials keep that id.
///
/// @details A shader is read and compiled when it is asked for (detail::compileShader()), on every side; the
/// ShaderPlugin hands the compiled ones to the renderer on the client. A shader that cannot be read or compiled is
/// recorded as failed, with the reason, which is also printed: one broken shader does not stop the game.
/// @code
/// const rtype::core::asset::AssetId flash = shaders->load("assets/shaders/flash.glsl");
/// @endcode
class ShaderAssets {
  public:
    /// @brief One shader: its file (or the name it was added under), its compiled form (nothing if it failed) and why.
    using Entry = core::asset::AssetStore<backend::graphics::CompiledShader>::Entry;

    ShaderAssets() = default;
    ~ShaderAssets() = default;
    ShaderAssets(const ShaderAssets& other) = delete;
    ShaderAssets& operator=(const ShaderAssets& other) = delete;
    ShaderAssets(ShaderAssets&& other) noexcept = default;
    ShaderAssets& operator=(ShaderAssets&& other) noexcept = default;

    /// @return The number of shaders known, compiled or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Reads and compiles the shader file at @p path (the engine's GLSL dialect, docs/Shaders.md).
    /// @return Its id (AssetId::fromPath()), at once, even if it failed. Asking twice gives the first one.
    /// @throws core::asset::exceptions::AssetLoadException If another path already has this id (a hash collision).
    core::asset::AssetId load(const std::filesystem::path& path);

    /// @brief Compiles a shader written in code, known by @p name.
    /// @return Its id (AssetId::fromPath(name)), even if it failed. Adding a name twice keeps the first shader.
    /// @throws core::asset::exceptions::AssetLoadException If another name already has this id (a hash collision).
    core::asset::AssetId add(const std::string& name, std::string_view source);

    /// @return The shader @p id names, compiled, or nullptr if it is unknown or failed.
    [[nodiscard]] const backend::graphics::CompiledShader* get(core::asset::AssetId id) const;

    /// @return The entry of @p id, compiled or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(core::asset::AssetId id) const;

    /// @brief Forgets the shader @p id names; the ShaderPlugin destroys it in the renderer.
    /// @return False, doing nothing, if no shader has this id.
    bool unload(core::asset::AssetId id);

    /// @return The shaders compiled since the last call, for the ShaderPlugin to create.
    [[nodiscard]] std::vector<core::asset::AssetId> takeAdded();

    /// @return The shaders unloaded since the last call, for the ShaderPlugin to destroy.
    [[nodiscard]] std::vector<core::asset::AssetId> takeUnloaded();

  private:
    core::asset::AssetStore<backend::graphics::CompiledShader> _shaders{"Shader"};  ///< Every shader, by id.
    std::vector<core::asset::AssetId> _added;     ///< Compiled since the last takeAdded().
    std::vector<core::asset::AssetId> _unloaded;  ///< Unloaded since the last takeUnloaded().
};
RTYPE_ECS_RESOURCE(ShaderAssets, "rtype.ShaderAssets");

}  // namespace rtype::plugins::shader
