/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureCalls
*/

#include "plugins/graphics/TextureCalls.hpp"

#include <filesystem>
#include <string>

#include "plugins/graphics/TextureAssets.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::graphics {
namespace {
TextureResult loadTexture(ecs::ResMut<TextureAssets> textures, const LoadTextureArguments& arguments) {
    const contract::renderer::TextureDesc desc{
        .filter =
            arguments.linear ? contract::renderer::TextureFilter::kLinear : contract::renderer::TextureFilter::kNearest,
        .repeat = arguments.wrap,
    };
    return TextureResult{.texture = textures->load(std::filesystem::path{arguments.path.view()}, desc)};
}
}  // namespace

void addTextureCalls(ecs::App& app) { app.addCall(std::string{kLoadTextureCall}, &loadTexture); }

}  // namespace rtype::plugins::graphics
