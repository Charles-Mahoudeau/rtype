/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderPlugin
*/

#include "plugins/shader/ShaderPlugin.hpp"

#include <string>
#include <string_view>

#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResource.hpp"  // IWYU pragma: keep
#include "plugins/backend/Names.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
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
void updateShaders(ecs::NonSendMut<backend::Backend> backend, ecs::ResMut<ShaderAssets> shaders,
                   ecs::ResMut<MaterialAssets> materials, ecs::Res<backend::TextureAssets> textures,
                   ecs::ResMut<ShaderHandles> handles) {
    if (backend->renderer != nullptr) {
        handles->update(*shaders, *materials, *textures, *backend->renderer);
    }
}
}  // namespace

std::string_view ShaderPlugin::getName() const noexcept { return kName; }

void ShaderPlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<ShaderAssets>();
    resources.insertIfAbsent<MaterialAssets>();
    resources.insertIfAbsent<ShaderHandles>();
    resources.insertIfAbsent<backend::TextureAssets>();
    static_cast<void>(app.getWorld().getComponents().registerType<MaterialInstance>());
    app.addSystem(ecs::Phase::kRender, std::string{kUpdateSystem}, &updateShaders)
        .after(std::string{backend::names::kBackendTextures})
        .before(std::string{backend::names::kBackendBeginFrame})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::shader
