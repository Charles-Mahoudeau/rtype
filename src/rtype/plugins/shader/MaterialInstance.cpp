/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialInstance
*/

#include "plugins/shader/MaterialInstance.hpp"

#include <string>

#include "plugins/shader/Material.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::plugins::shader {

MaterialInstance MaterialInstance::of(const contract::renderer::CompiledShader& shader) {
    return MaterialInstance{.values = shader.instanceDefaults};
}

MaterialInstance& MaterialInstance::set(const contract::renderer::CompiledShader& shader, const std::string& name,
                                        const MaterialValue& value) {
    writeInstanceValue(shader, values, name, value);
    return *this;
}

}  // namespace rtype::plugins::shader
