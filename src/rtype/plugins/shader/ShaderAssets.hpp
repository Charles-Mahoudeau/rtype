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
#include <unordered_set>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/asset/AssetStore.hpp"
#include "plugins/shader/Export.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::shader {

/// @brief The game's shaders, compiled, as the ECS resource "rtype.ShaderAssets" the ShaderPlugin inserts: systems ask
/// for a shader and get its AssetId at once, materials keep that id.
///
/// @details A shader is read and compiled when it is asked for (detail::compileShader()), on every side; the
/// ShaderPlugin hands the compiled ones to the renderer on the client. A shader that cannot be read or compiled is
/// recorded as failed, with the reason, which is also printed: one broken shader does not stop the game. A shader file
/// keeps its id when it is compiled again (reload()), so the materials that name it follow it.
/// @code
/// const rtype::core::asset::AssetId flash = shaders->load("assets/shaders/flash.glsl");
/// @endcode
class RTYPE_PLUGIN_SHADER_API ShaderAssets {
  public:
    /// @brief One shader: its file (or the name it was added under), its compiled form (nothing if it failed) and why.
    using Entry = core::asset::AssetStore<contract::renderer::CompiledShader>::Entry;

    ShaderAssets() = default;
    ~ShaderAssets() = default;
    ShaderAssets(const ShaderAssets& other) = delete;
    ShaderAssets& operator=(const ShaderAssets& other) = delete;
    ShaderAssets(ShaderAssets&& other) noexcept = default;
    ShaderAssets& operator=(ShaderAssets&& other) noexcept = default;

    /// @return The number of shaders known, compiled or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Sets what the paths given to load() are relative to, for the shaders loaded from now on.
    /// A path that leaves it fails (core::asset::resolveAssetPath()).
    /// @param directory The assets' directory; empty: the working directory, unchecked.
    void setDirectory(std::filesystem::path directory);

    /// @brief Reads and compiles the shader file at @p path (the engine's GLSL dialect, docs/Shaders.md).
    /// @param path Relative to setDirectory()'s directory. It names the shader as it is given.
    /// @return Its id (AssetId::fromPath()), at once, even if it failed. Asking twice gives the first one.
    /// @throws core::asset::exceptions::AssetLoadException If another path already has this id (a hash collision).
    core::asset::AssetId load(const std::filesystem::path& path);

    /// @brief Compiles a shader written in code, known by @p name.
    /// @return Its id (AssetId::fromPath(name)), even if it failed. Adding a name twice keeps the first shader.
    /// @throws core::asset::exceptions::AssetLoadException If another name already has this id (a hash collision).
    core::asset::AssetId add(const std::string& name, std::string_view source);

    /// @return The shader @p id names, compiled, or nullptr if it is unknown or failed.
    [[nodiscard]] const contract::renderer::CompiledShader* get(core::asset::AssetId id) const;

    /// @return The entry of @p id, compiled or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(core::asset::AssetId id) const;

    /// @brief Reads and compiles again the file of the shader @p id names. If that fails, the reason is printed and a
    /// shader already compiled stays as it was, so a typo does not take it off the screen.
    /// @return Whether the shader was replaced: false if it failed, or if @p id names no file (add()).
    bool reload(core::asset::AssetId id);

    /// @brief Forgets the shader @p id names; the ShaderPlugin destroys it in the renderer.
    /// @return False, doing nothing, if no shader has this id.
    bool unload(core::asset::AssetId id);

    /// @return The shaders compiled since the last call, for the ShaderPlugin to create.
    [[nodiscard]] std::vector<core::asset::AssetId> takeAdded();

    /// @return The shaders replaced by reload() since the last call, each once, for the ShaderPlugin to create again.
    [[nodiscard]] std::vector<core::asset::AssetId> takeReloaded();

    /// @return The shaders unloaded since the last call, for the ShaderPlugin to destroy.
    [[nodiscard]] std::vector<core::asset::AssetId> takeUnloaded();

  private:
    /// @return The shader file @p name, read and compiled.
    /// @throws std::exception If it cannot be read or compiled, with a message that names it.
    [[nodiscard]] contract::renderer::CompiledShader compileFile(const std::string& name) const;

    core::asset::AssetStore<contract::renderer::CompiledShader> _shaders{"Shader"};  ///< Every shader, by id.
    std::filesystem::path _directory;                 ///< What the paths given to load() are relative to.
    std::unordered_set<core::asset::AssetId> _files;  ///< The shaders load() read from a file.
    std::vector<core::asset::AssetId> _added;         ///< Compiled since the last takeAdded().
    std::vector<core::asset::AssetId> _reloaded;      ///< Replaced since the last takeReloaded().
    std::vector<core::asset::AssetId> _unloaded;      ///< Unloaded since the last takeUnloaded().
};
RTYPE_ECS_RESOURCE(ShaderAssets, "rtype.ShaderAssets");

}  // namespace rtype::plugins::shader
