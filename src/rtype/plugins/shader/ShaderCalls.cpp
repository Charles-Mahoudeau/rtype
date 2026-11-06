/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCalls
*/

#include "plugins/shader/ShaderCalls.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::shader {
namespace {
/// @return Whether the shader of @p material declares the `uniform` @p name as an `int`.
[[nodiscard]] bool isInt(const ShaderAssets& shaders, const MaterialAssets& materials,
                         const core::asset::AssetId material, const std::string_view name) {
    const Material* found = materials.get(material);
    const contract::renderer::CompiledShader* shader = found != nullptr ? shaders.get(found->shader) : nullptr;
    if (shader == nullptr) {
        return false;
    }
    const auto declared = std::ranges::find(shader->materialValues, name, &contract::renderer::ShaderValue::name);
    return declared != shader->materialValues.end() && declared->type == contract::renderer::ShaderValueType::kInt;
}

/// @return The value @p arguments describe; a script has one kind of number, so it is an `int` if @p integer says so
/// and it is whole.
/// @throws exceptions::ShaderException If its size is not 1 to 4.
[[nodiscard]] MaterialValue readValue(const SetMaterialValueArguments& arguments, const bool integer) {
    switch (arguments.size) {
        case 1:
            if (integer && std::trunc(arguments.x) == arguments.x) {
                return static_cast<std::int32_t>(arguments.x);
            }
            return arguments.x;
        case 2:
            return glm::vec2{arguments.x, arguments.y};
        case 3:
            return glm::vec3{arguments.x, arguments.y, arguments.z};
        case 4:
            return glm::vec4{arguments.x, arguments.y, arguments.z, arguments.w};
        default:
            throw exceptions::ShaderException{
                std::format("A material value has 1 to 4 components, not {}", arguments.size)};
    }
}

ShaderResult loadShader(ecs::ResMut<ShaderAssets> shaders, const LoadShaderArguments& arguments) {
    return ShaderResult{.shader = shaders->load(std::filesystem::path{arguments.path.view()})};
}

MaterialResult addMaterial(ecs::ResMut<MaterialAssets> materials, const AddMaterialArguments& arguments) {
    return MaterialResult{.material =
                              materials->add(std::string{arguments.name.view()}, Material{.shader = arguments.shader})};
}

MaterialResult setMaterialValue(ecs::Res<ShaderAssets> shaders, ecs::ResMut<MaterialAssets> materials,
                                const SetMaterialValueArguments& arguments) {
    const bool integer = isInt(*shaders, *materials, arguments.material, arguments.name.view());
    if (!materials->setValue(arguments.material, std::string{arguments.name.view()}, readValue(arguments, integer))) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    return MaterialResult{.material = arguments.material};
}

MaterialResult setMaterialTexture(ecs::ResMut<MaterialAssets> materials, const SetMaterialTextureArguments& arguments) {
    if (!materials->setTexture(arguments.material, std::string{arguments.name.view()}, arguments.texture)) {
        throw exceptions::ShaderException{std::format("No material has the id {}", arguments.material.value)};
    }
    return MaterialResult{.material = arguments.material};
}
}  // namespace

void addShaderForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "ShaderForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addShaderForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addShaderCalls(ecs::App& app) {
    addShaderForms(app.getPluginRegistry());
    app.addCall(std::string{kLoadShaderCall}, &loadShader);
    app.addCall(std::string{kAddMaterialCall}, &addMaterial);
    app.addCall(std::string{kSetValueCall}, &setMaterialValue);
    app.addCall(std::string{kSetTextureCall}, &setMaterialTexture);
}

}  // namespace rtype::plugins::shader
