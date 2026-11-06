/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialInstance
*/

#include "plugins/backend/graphics/MaterialInstance.hpp"

#include <string>

#include "plugins/backend/graphics/MaterialValue.hpp"
#include "plugins/backend/graphics/Shader.hpp"

namespace rtype::plugins::backend::graphics {

MaterialInstance MaterialInstance::of(const CompiledShader& shader) {
    return MaterialInstance{.values = shader.instanceDefaults};
}

MaterialInstance& MaterialInstance::set(const CompiledShader& shader, const std::string& name,
                                        const MaterialValue& value) {
    writeInstanceValue(shader, values, name, value);
    return *this;
}

}  // namespace rtype::plugins::backend::graphics
