/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/shader/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::shader {

/// @brief Game shaders and materials: inserts the ShaderAssets, MaterialAssets and ShaderHandles resources, registers
/// MaterialInstance, and keeps the renderer up to date with them.
///
/// @details One system: Render "rtype.Shader.Update", on the client, after the GraphicsPlugin loads the textures and
/// before it begins the frame: ShaderHandles::update() creates the shaders and materials added during this frame's
/// Update, so they are drawn in this frame, and destroys the unloaded ones. A SpriteRenderer names its material by
/// AssetId; the GraphicsPlugin draws it with the material ShaderHandles created, or the default one until then.
/// It requires the GraphicsPlugin. Its system is ordered against the GraphicsPlugin's, so an App without a window (a
/// server) does not add it: it inserts ShaderAssets and MaterialAssets itself if its gameplay code names materials.
///
/// In a game with hot reload on (the `hotReload` setting), it adds a ShaderReloadable to the HotReload resource, which
/// follows `assets/shaders` of the game. Without an AssetDirectory or a HotReload resource, nothing is followed.
/// @code
/// const rtype::core::asset::AssetId flash = shaders->load("assets/shaders/flash.glsl");
/// const rtype::core::asset::AssetId hit =
///     materials->add("enemy_hit", rtype::plugins::shader::Material{.shader = flash}.set("strength", 0.8F));
/// commands.spawn(rtype::plugins::sprite::SpriteRenderer{.texture = enemy, .material = hit});
/// @endcode
class RTYPE_PLUGIN_SHADER_API ShaderPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Shader";                 ///< The plugin's name.
    static constexpr std::string_view kUpdateSystem = "rtype.Shader.Update";  ///< Updates the renderer.

    ShaderPlugin() = default;
    ~ShaderPlugin() override = default;
    ShaderPlugin(const ShaderPlugin& other) = default;
    ShaderPlugin& operator=(const ShaderPlugin& other) = default;
    ShaderPlugin(ShaderPlugin&& other) noexcept = default;
    ShaderPlugin& operator=(ShaderPlugin&& other) noexcept = default;

    /// @return "rtype.Shader".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The plugins this one needs.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Inserts the resources and adds the system.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::shader
