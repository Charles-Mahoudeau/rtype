/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialInstance
*/

#include "plugins/shader/MaterialInstance.hpp"

#include <string>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/Material.hpp"

namespace rtype::plugins::shader {

MaterialInstance MaterialInstance::of(const backend::graphics::CompiledShader& shader) {
    return MaterialInstance{.values = shader.instanceDefaults};
}

MaterialInstance& MaterialInstance::set(const backend::graphics::CompiledShader& shader, const std::string& name,
                                        const MaterialValue& value) {
    writeInstanceValue(shader, values, name, value);
    return *this;
}

}  // namespace rtype::plugins::shader
