/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialInstance
*/

#pragma once

#include "plugins/backend/graphics/MaterialInstance.hpp"

namespace rtype::plugins::shader {
/// @brief An entity's own `instance uniform` values: the contract's component "rtype.MaterialInstance", which the
/// plugins that draw read without knowing this plugin.
using MaterialInstance = backend::graphics::MaterialInstance;
}  // namespace rtype::plugins::shader
