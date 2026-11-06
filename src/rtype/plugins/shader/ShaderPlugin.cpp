/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderPlugin
*/

#include "plugins/shader/ShaderPlugin.hpp"

#include <array>
#include <span>
#include <string>
#include <string_view>

#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"  // IWYU pragma: keep
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::shader {

namespace {
/// @brief Render, client: creates, updates and destroys the shaders and materials in the renderer.
void updateShaders(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::ResMut<ShaderAssets> shaders,
                   ecs::ResMut<MaterialAssets> materials, ecs::Res<graphics::TextureAssets> textures,
                   ecs::ResMut<ShaderHandles> handles) {
    if (backend->renderer != nullptr) {
        handles->update(*shaders, *materials, *textures, *backend->renderer);
    }
}
}  // namespace

std::string_view ShaderPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> ShaderPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 1> kDependencies{graphics::GraphicsPlugin::kName};
    return kDependencies;
}

void ShaderPlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<ShaderAssets>();
    resources.insertIfAbsent<MaterialAssets>();
    resources.insertIfAbsent<ShaderHandles>();
    resources.insertIfAbsent<graphics::TextureAssets>();
    static_cast<void>(app.getWorld().getComponents().registerType<MaterialInstance>());
    app.addSystem(ecs::Phase::kRender, std::string{kUpdateSystem}, &updateShaders)
        .after(std::string{graphics::GraphicsPlugin::kTexturesSystem})
        .before(std::string{graphics::GraphicsPlugin::kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::shader
