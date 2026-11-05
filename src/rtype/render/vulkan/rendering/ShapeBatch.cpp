/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapeBatch
*/

#include "ShapeBatch.hpp"

#include <cstdint>

#include "ShapeInstance.hpp"
#include "plugins/backend/graphics/Shader.hpp"

namespace rtype::render::vulkan::rendering {

void ShapeBatch::add(const ShapeInstance& instance) {
    extend({});
    _instances.push_back(instance);
    ++_batches.back().instanceCount;
}

void ShapeBatch::add(plugins::backend::graphics::MaterialId material, const ShapeInstance& instance,
                     const plugins::backend::graphics::ShaderInstanceValues& values) {
    extend(material);
    _instances.push_back(instance);
    if (material != plugins::backend::graphics::MaterialId{}) {
        _values.push_back(values);
    }
    ++_batches.back().instanceCount;
}

void ShapeBatch::clear() noexcept {
    _instances.clear();
    _values.clear();
    _batches.clear();
}

void ShapeBatch::extend(plugins::backend::graphics::MaterialId material) {
    if (_batches.empty() || _batches.back().material != material) {
        _batches.push_back(Batch{.material = material,
                                 .firstInstance = static_cast<std::uint32_t>(_instances.size()),
                                 .firstValues = static_cast<std::uint32_t>(_values.size())});
    }
}

}  // namespace rtype::render::vulkan::rendering
